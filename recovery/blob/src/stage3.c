#include <baremetal/cdram.h>
#include <baremetal/ctrl.h>
#include <baremetal/display.h>
#include <baremetal/gpio.h>
#include <baremetal/i2c.h>
#include <baremetal/msif.h>
#include <baremetal/pervasive.h>
#include <baremetal/syscon.h>
#include <baremetal/sysroot.h>
#include <baremetal/touch.h>
#include <baremetal/utils.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../../../core/enso.h"
#include "../../../core/ex_defs.h"
#include "bm_ext.h"
#include "main.h"
#include "nskbl.h"
#include "paper.h"
#include "stage3.h"
#include "stor.h"
#include "utils.h"
#include "view.h"
#include "fmgr.h"
#include "ff.h"

struct menu_s stage3_menu_s = {.draw = stage3_menu,
                               .select = stage3_menu,
                               .entry_count = 5,
                               .selection = 0,
                               .exp_buttons = CTRL_CROSS,
                               .prs_buttons = 0,
                               .paper = &menu_paper,
                               .selector_color = MENU_SELECTOR_COLR};

static void s3_swap_os0(void) {
    view_switch(VIEW_DEFAULT);  // switch to the log view
    ULOG("Swapping active os0 in emuMBR...\n");
    master_block_t mbr;
    memset(&mbr, 0, sizeof(mbr));
    int ret = EMMCREAD(ENSO_EMUMBR_OFFSET, &mbr, sizeof(mbr) / SECTOR_SIZE);
    if (ret < 0) {
        ULOG("Failed to read emuMBR: 0x%08X\n", ret);
        return;
    }
    if (mbr.sig != 0xAA55) {
        ULOG("Invalid emuMBR signature: 0x%04X\n", mbr.sig);
        return;
    }
    partition_t *active_os0 = stor_find_partition_by_id(&mbr, STOR_PART_OS, STOR_PART_ACTIVE_YES);
    partition_t *inactive_os0 = stor_find_partition_by_id(&mbr, STOR_PART_OS, STOR_PART_ACTIVE_NOT);
    if (!active_os0 || !inactive_os0) {
        ULOG("Failed to find both os0 partitions\n");
        return;
    }
    active_os0->active = 0;
    inactive_os0->active = 1;
    ret = EMMCWRITE(ENSO_EMUMBR_OFFSET, &mbr, sizeof(mbr) / SECTOR_SIZE);
    if (ret < 0)
        ULOG("Failed to write emuMBR: 0x%08X\n", ret);
    else
        ULOG("Successfully swapped active os0 in emuMBR\nReboot for changes to take effect.\n");
    view_switch(VIEW_MENU);  // switch back to the menu view
}

static void s3_rwudi_sqh(enum FMGR_ENTRY_TYPES entype, char *path, char *entry, enum VIEW_ASSIGNS *next_uview) {
    view_switch(VIEW_DEFAULT);  // switch to the log view
    ILOG("s3_rwudi_sqh(entype=%d, path=%s, entry=%s)\n", entype, path, entry);
    int ret = idstorage_init();
    if (ret >= 0) {
        void *buf = my_malloc((2 * SECTOR_SIZE) + (2 * FMGR_MAX_PATH_LEN));
        if (buf) {
            if (entype == FMGR_ENTRY_TYPE_DIR) {  // dump UDIs to dir
                void *tmpb = buf + SECTOR_SIZE;
                char *fpath = tmpb + SECTOR_SIZE;
                ret = idstorage_rw_leaf(0, S3_LEAF_CID, tmpb);
                if (ret >= 0) {
                    memcpy(buf, tmpb + 0xA0, 0x100);
                    ret = idstorage_rw_leaf(0, S3_LEAF_OPSID_0, tmpb);
                    if (ret >= 0) {
                        memcpy(buf + 0x100, tmpb + 0x128, SECTOR_SIZE - 0x128);
                        ret = idstorage_rw_leaf(0, S3_LEAF_OPSID_1, tmpb);
                        if (ret >= 0) {
                            memcpy(buf + 0x100 + (SECTOR_SIZE - 0x128), tmpb, 0x100 - (SECTOR_SIZE - 0x128));
                            my_snprintf(fpath, (2 * FMGR_MAX_PATH_LEN), "%s%s%s", path, entry, S3_UDI_OUTFNAME);
                            ret = fmgr_set_file(fpath, buf, SECTOR_SIZE, NULL);
                            if (ret >= 0)
                                ULOG("UDIs successfully dumped to %s\n", fpath);
                            else
                                ULOG("Failed to write UDIs to file: 0x%08X\n", ret);
                        } else
                            ULOG("Failed to read OpenPSID leaf 1: 0x%08X\n", ret);
                    } else
                        ULOG("Failed to read OpenPSID leaf 0: 0x%08X\n", ret);
                } else
                    ULOG("Failed to read CID leaf: 0x%08X\n", ret);
            } else {
                void *leaf2 = buf + SECTOR_SIZE;
                char *fpath = leaf2 + SECTOR_SIZE;
                my_snprintf(fpath, (2 * FMGR_MAX_PATH_LEN), "%s%s", path, entry);
                uint32_t f_br = 0;
                void *fdata = fmgr_get_file(fpath, NULL, 0, 0, &f_br);
                if (fdata) {
                    uint32_t fcrc = crc32(0, fdata, f_br);
                    if (f_br == SECTOR_SIZE) {
                        lpreset();
                        lpclog(RED, "WARNING: ");
                        lplog("Writing invalid data WILL brick the device!\n");
                        lplog("Are you sure that you want to continue?\n");
                        lplog("New UDIs CRC32: 0x%08X\n", fcrc);
                        lpclog(GREEN, "Press CIRCLE to continue, CROSS to cancel.\n");
                        ret = bmx_ctrl_wait(CTRL_CIRCLE | CTRL_CROSS, 4000, 1);
                        if (BMX_CTRL_BUTTON_HELD(ret, CTRL_CIRCLE)) {
                            ret = idstorage_rw_leaf(0, S3_LEAF_CID, buf);
                            if (ret >= 0) {
                                memcpy(buf + 0xA0, fdata, 0x100);
                                ret = idstorage_rw_leaf(1, S3_LEAF_CID, buf);
                                if (ret >= 0) {
                                    ULOG("Wrote CID leaf\n");
                                    ret = idstorage_rw_leaf(0, S3_LEAF_OPSID_0, buf);
                                    if (ret >= 0) {
                                        ret = idstorage_rw_leaf(0, S3_LEAF_OPSID_1, leaf2);
                                        if (ret >= 0) {
                                            memcpy(buf + 0x128, fdata + 0x100, 0x100);
                                            ret = idstorage_rw_leaf(1, S3_LEAF_OPSID_0, buf);
                                            if (ret >= 0) {
                                                ULOG("Wrote OpenPSID leaf 0\n");
                                                ret = idstorage_rw_leaf(1, S3_LEAF_OPSID_1, leaf2);
                                                if (ret >= 0) {
                                                    ULOG("Wrote OpenPSID leaf 1\n");
                                                    ULOG("UDIs written successfully\n");
                                                } else
                                                    ULOG("Failed to write OpenPSID leaf 1: 0x%08X\n", ret);
                                            } else
                                                ULOG("Failed to write OpenPSID leaf 0: 0x%08X\n", ret);
                                        } else
                                            ULOG("Failed to read OpenPSID leaf 1: 0x%08X\n", ret);
                                    } else
                                        ULOG("Failed to read OpenPSID leaf 0: 0x%08X\n", ret);
                                } else
                                    ULOG("Failed to write CID leaf: 0x%08X\n", ret);
                            } else
                                ULOG("Failed to read CID leaf: 0x%08X\n", ret);
                        } else
                            ULOG("Operation cancelled by user.\n");
                    } else
                        ULOG("File %s has unexpected size: %d\n", fpath, f_br);
                    my_free(fdata);
                } else
                    ULOG("Failed to read file %s\n", fpath);
            }
            my_free(buf);
        } else
            ULOG("Failed to allocate memory for buffer\n");
        idstorage_stop();
    } else
        ULOG("Failed to initialize ID storage: 0x%08X\n", ret);
    *next_uview = VIEW_MENU;
    fmgr_square_handler(1, 0, NULL);
}

int stage3_menu(int selection) {
    if (selection < 0) {  // initial draw
        paper_clear(stage3_menu_s.paper, stage3_menu_s.paper->color);
        pen_reset(stage3_menu_s.paper, stage3_menu_s.paper->pen.color);
        pprintf(stage3_menu_s.paper, "1. Continue boot\n");
        pprintf(stage3_menu_s.paper, "2. Swap active os0 in emuMBR\n");
        pprintf(stage3_menu_s.paper, "3. Dump/Flash Unique Device IDs\n");
        pprintf(stage3_menu_s.paper, "4. Reboot the system\n");
        pprintf(stage3_menu_s.paper, "5. Power off the device\n");
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stage3_menu_s.prs_buttons, CTRL_CROSS)) {
        switch (selection) {
            case 0:  // Continue boot
                ULOG("Continuing boot...\n");
                return MENU_RET_FINISH_DEINIT;
            case 1: // Swap active os0 in emuMBR
                s3_swap_os0();
                return MENU_RET_CONTINUE;
            case 2: // Dump/Flash Unique Device IDs
                if (fmgr_square_handler(0, 0, NULL) != s3_rwudi_sqh) {
                    ULOG("Use [] in the file manager to select source file or destination directory for UDIs.\n");
                    ULOG("Press X again to cancel.\n");
                    fmgr_square_handler(1, BITN(FMGR_ENTRY_TYPE_DIR) | BITN(FMGR_ENTRY_TYPE_FILE), s3_rwudi_sqh);
                } else {
                    ULOG("Canceling the UDI operation.\n");
                    fmgr_square_handler(1, 0, NULL);
                }
                return MENU_RET_CONTINUE;
            case 3: // Reboot the system
                ULOG("Rebooting the system...\n");
                syscon_reset_device(SYSCON_RESET_TYPE_COLD_RESET, 0);
                return MENU_RET_CONTINUE;
            case 4: // Power off the device
                ULOG("Powering off the device...\n");
                syscon_reset_device(SYSCON_RESET_TYPE_POWEROFF, 0);
                return MENU_RET_CONTINUE;
            default:
                ELOG("Invalid selection %d\n", selection);
                return MENU_RET_CONTINUE;  // continue the loop
        }
    };
    return MENU_RET_CONTINUE;  // continue the loop
}

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
                               .entry_count = 6,
                               .selection = 0,
                               .exp_buttons = CTRL_CROSS,
                               .prs_buttons = 0,
                               .paper = &menu_paper,
                               .selector_color = MENU_SELECTOR_COLR};

static void s3_uprecovery_sqh(enum FMGR_ENTRY_TYPES entype, char *path, char *entry, enum VIEW_ASSIGNS *next_uview) {
    int ret = 0;
    void *buf = NULL;
    char fpath[2 * FMGR_MAX_PATH_LEN];
    view_switch(VIEW_DEFAULT);  // switch to the log view
    alllog("Updating the enso_ex recovery from %s%s (DIR)...\n", path, entry);

    my_snprintf(fpath, sizeof(fpath), "%s%s%s", path, entry, STAGE3_UPR_MBR_FNAME);
    buf = fmgr_get_file(fpath, NULL, SECTOR_SIZE, 0);
    if (buf) {
        LOG("Flashing recovery MBR from %s\n", fpath);
        ret = EMMCWRITE(E2X_RECOVERY_MBR_OFFSET, buf, 1);
        if (ret < 0)
            alllog("Failed to write recovery MBR to EMMC: 0x%08X\n", ret);
        else
            alllog("Successfully wrote recovery MBR to EMMC\n");
        my_free(buf);
    } else
        alllog("No recovery MBR found at %s or error\n", fpath);

    my_snprintf(fpath, sizeof(fpath), "%s%s%s", path, entry, STAGE3_UPR_CONFIG_FNAME);
    buf = fmgr_get_file(fpath, NULL, E2X_RCONF_SIZE, 0);
    if (buf) {
        LOG("Flashing recovery config from %s\n", fpath);
        ret = EMMCWRITE(E2X_RCONF_OFFSET, buf, E2X_RCONF_SIZE / SECTOR_SIZE);
        if (ret < 0)
            alllog("Failed to write recovery config to EMMC: 0x%08X\n", ret);
        else
            alllog("Successfully wrote recovery config to EMMC\n");
        my_free(buf);
    } else
        alllog("No recovery config found at %s or error\n", fpath);

    my_snprintf(fpath, sizeof(fpath), "%s%s%s", path, entry, STAGE3_UPR_BLOB_FNAME);
    buf = fmgr_get_file(fpath, NULL, STAGE3_UPR_BUFSIZE, 0);
    if (buf) {
        LOG("Flashing recovery blob from %s\n", fpath);
        ret = EMMCWRITE(E2X_RBLOB_OFFSET, buf, E2X_RBLOB_SIZE / SECTOR_SIZE);
        if (ret < 0)
            alllog("Failed to write recovery blob to EMMC: 0x%08X\n", ret);
        else
            alllog("Successfully wrote recovery blob to EMMC\n");
        my_free(buf);
    } else
        alllog("No recovery blob found at %s or error\n", fpath);

    alllog("Recovery update procedure complete, switching back to main view\n");
    *next_uview = VIEW_MENU;  // switch back to the menu view
    fmgr_square_handler(1, 0, NULL);
}

static void s3_upsecond_sqh(enum FMGR_ENTRY_TYPES entype, char *path, char *entry, enum VIEW_ASSIGNS *next_uview) {
    int ret = 0;
    void *buf = NULL;
    char fpath[2 * FMGR_MAX_PATH_LEN];
    view_switch(VIEW_DEFAULT);  // switch to the log view
    alllog("Updating the enso_ex stage 2 from %s%s (FILE)...\n", path, entry);

    my_snprintf(fpath, sizeof(fpath), "%s%s", path, entry);
    buf = fmgr_get_file(fpath, NULL, SECOND_PAYLOAD_SIZE, 0);
    if (buf) {
        LOG("Flashing stage 2 from %s\n", fpath);
        ret = EMMCWRITE(SECOND_PAYLOAD_OFFSET, buf, SECOND_PAYLOAD_SIZE / SECTOR_SIZE);
        if (ret < 0)
            alllog("Failed to write stage 2 to EMMC: 0x%08X\n", ret);
        else
            alllog("Successfully wrote stage 2 to EMMC\n");
        my_free(buf);
    } else
        alllog("No stage 2 found at %s or error\n", fpath);

    alllog("Stage 2 update procedure complete, switching back to main view\n");
    *next_uview = VIEW_MENU;  // switch back to the menu view
    fmgr_square_handler(1, 0, NULL);
}

static void s3_emmcdump_sqh(enum FMGR_ENTRY_TYPES entype, char *path, char *entry, enum VIEW_ASSIGNS *next_uview) {
    view_switch(VIEW_DEFAULT);  // switch to the log view
    alllog("Dumping eMMC to file %s%s...\n", path, entry);

    master_block_t mbr;
    memset(&mbr, 0, sizeof(mbr));
    int ret = EMMCREAD(0, &mbr, sizeof(mbr) / SECTOR_SIZE);
    if (ret >= 0) {
        if (mbr.sig == 0xAA55) {
            char dest_path[FMGR_MAX_PATH_LEN];
            my_snprintf(dest_path, sizeof(dest_path), "%s%s", path, entry);
            ret = fmgr_raw_dump(0, mbr.device_size, dest_path);
            if (ret < 0)
                alllog("Failed to dump eMMC: 0x%08X\n", ret);
            else
                alllog("Successfully dumped eMMC to %s%s\n", path, entry);
        } else
            alllog("Invalid MBR signature: 0x%04X\n", mbr.sig);
    } else
        alllog("Failed to read MBR: 0x%08X\n", ret);
    
    *next_uview = VIEW_MENU;  // switch back to the menu view
    fmgr_square_handler(1, 0, NULL);
}

static void s3_swap_os0(void) {
    view_switch(VIEW_DEFAULT);  // switch to the log view
    alllog("Swapping active os0 in emuMBR...\n");
    master_block_t mbr;
    memset(&mbr, 0, sizeof(mbr));
    int ret = EMMCREAD(ENSO_EMUMBR_OFFSET, &mbr, sizeof(mbr) / SECTOR_SIZE);
    if (ret < 0) {
        alllog("Failed to read emuMBR: 0x%08X\n", ret);
        return;
    }
    if (mbr.sig != 0xAA55) {
        alllog("Invalid emuMBR signature: 0x%04X\n", mbr.sig);
        return;
    }
    partition_t *active_os0 = stor_find_partition_by_id(&mbr, STOR_PART_OS, STOR_PART_ACTIVE_YES);
    partition_t *inactive_os0 = stor_find_partition_by_id(&mbr, STOR_PART_OS, STOR_PART_ACTIVE_NOT);
    if (!active_os0 || !inactive_os0) {
        alllog("Failed to find both os0 partitions\n");
        return;
    }
    active_os0->active = 0;
    inactive_os0->active = 1;
    ret = EMMCWRITE(ENSO_EMUMBR_OFFSET, &mbr, sizeof(mbr) / SECTOR_SIZE);
    if (ret < 0)
        alllog("Failed to write emuMBR: 0x%08X\n", ret);
    else
        alllog("Successfully swapped active os0 in emuMBR\nReboot for changes to take effect.\n");
    view_switch(VIEW_MENU);  // switch back to the menu view
}

int stage3_menu(int selection) {
    if (selection < 0) {  // initial draw
        paper_clear(stage3_menu_s.paper, stage3_menu_s.paper->color);
        pen_reset(stage3_menu_s.paper, stage3_menu_s.paper->pen.color);
        pprintf(stage3_menu_s.paper, "1. Continue boot\n");
        pprintf(stage3_menu_s.paper, "2. Un/Block writes to boot sectors\n");
        pprintf(stage3_menu_s.paper, "3. Update the enso_ex recovery\n");
        pprintf(stage3_menu_s.paper, "4. Update enso_ex's stage 2 payload\n");
        pprintf(stage3_menu_s.paper, "5. Dump the eMMC to file\n");
        pprintf(stage3_menu_s.paper, "6. Swap active os0 in emuMBR\n");
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stage3_menu_s.prs_buttons, CTRL_CROSS)) {
        switch (selection) {
            case 0:  // Continue boot
                alllog("Continuing boot...\n");
                return MENU_RET_FINISH_DEINIT;
            case 1:  // Un/Block writes to boot sectors
                if (g_disable_bootarea_update) {
                    g_disable_bootarea_update = 0;
                    DACR_OFF(*(g_eex_ports.protect_boot) = 0);
                    alllog("Boot area protection disabled\n");
                } else {
                    g_disable_bootarea_update = 1;
                    DACR_OFF(*(g_eex_ports.protect_boot) = 1);
                    alllog("Boot area protection enabled\n");
                }
                return MENU_RET_CONTINUE;
            case 2:  // Update the enso_ex recovery
                if (fmgr_square_handler(0, 0, NULL) != s3_uprecovery_sqh) {
                    alllog("Use [] in the file manager view to select a directory containing the enso_ex recovery files.\n");
                    alllog("Press X again to cancel the update.\n");
                    fmgr_square_handler(1, BITN(FMGR_ENTRY_TYPE_DIR), s3_uprecovery_sqh);
                } else {
                    alllog("Canceling the update.\n");
                    fmgr_square_handler(1, 0, NULL);
                }
                return MENU_RET_CONTINUE;
            case 3:  // Update the enso_ex's stage 2
                if (((struct sysroot_buffer*)(g_eex_ports.kbl_param))->boot_type_indicator_1 & 0x80004) {
                    if (fmgr_square_handler(0, 0, NULL) != s3_upsecond_sqh) {
                        alllog("Use [] in the file manager view to select a file containing the enso_ex stage 2 payload.\n");
                        alllog("Press X again to cancel the update.\n");
                        fmgr_square_handler(1, BITN(FMGR_ENTRY_TYPE_FILE), s3_upsecond_sqh);
                    } else {
                        alllog("Canceling the update.\n");
                        fmgr_square_handler(1, 0, NULL);
                    }
                } else
                    alllog("This option is only available in manufacturing mode.\n");
                return MENU_RET_CONTINUE;
            case 4:  // Dump the eMMC to file
                if (fmgr_square_handler(0, 0, NULL) != s3_emmcdump_sqh) {
                    alllog("Use [] in the file manager view to select a directory to dump the eMMC to.\n");
                    alllog("Press X again to cancel the dump.\n");
                    fmgr_square_handler(1, BITN(FMGR_ENTRY_TYPE_DIR), s3_emmcdump_sqh);
                } else {
                    alllog("Canceling the dump.\n");
                    fmgr_square_handler(1, 0, NULL);
                }
                return MENU_RET_CONTINUE;
            case 5: // Swap active os0 in emuMBR
                s3_swap_os0();
                return MENU_RET_CONTINUE;
            default:
                LOG("Invalid selection %d\n", selection);
                return MENU_RET_CONTINUE;  // continue the loop
        }
    };
    return MENU_RET_CONTINUE;  // continue the loop
}

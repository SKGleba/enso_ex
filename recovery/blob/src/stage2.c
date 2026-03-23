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
#include "stage2.h"
#include "stor.h"
#include "utils.h"
#include "fmgr.h"
#include "txtcfg.h"

struct menu_s stage2_menu_s = {
	.draw = stage2_menu,
	.select = stage2_menu,
	.entry_count = 6,
	.selection = 0,
	.exp_buttons = (CTRL_CROSS | CTRL_SQUARE | CTRL_START | CTRL_SELECT),
    .prs_buttons = 0,
    .paper = &menu_paper,
    .selector_color = MENU_SELECTOR_COLR
};

struct stage2_options stage2_opts = {
    .recovery_mbr = 0,
    .stage3_recovery = 0,
    .vanilla_boot = 0,
    .protect_bootarea = 0,
    .gcsd_mode = STAGE2_GCSD_MODE_DISABLED,
    .bootpatch = 0
};

static void stage2_status_update(void) {
    paper_clear(&status_paper, MENU_PAPER_COLR);
    pen_reset(&status_paper, STATUS_PEN_COLR);
    pprintf_align(&status_paper, LEFT, "S2:\n");
    if (stage2_opts.recovery_mbr)
        pprintf_align(&status_paper, RIGHT, "use R MBR <-\n");
    if (stage2_opts.stage3_recovery)
        pprintf_align(&status_paper, RIGHT, "goto s3 <-\n");
    if (stage2_opts.vanilla_boot)
        pprintf_align(&status_paper, RIGHT, "vanilla boot <-\n");
    if (stage2_opts.protect_bootarea)
        pprintf_align(&status_paper, RIGHT, "protect BOOT <-\n");
    if (stage2_opts.gcsd_mode != STAGE2_GCSD_MODE_DISABLED) {
        switch (stage2_opts.gcsd_mode) {
            case STAGE2_GCSD_MODE_SD0:
                pprintf_align(&status_paper, RIGHT, "GC-SD: sd0 <-\n");
                break;
            case STAGE2_GCSD_MODE_OS0:
                pprintf_align(&status_paper, RIGHT, "GC-SD: os0 <-\n");
                break;
            case STAGE2_GCSD_MODE_INIT:
                pprintf_align(&status_paper, RIGHT, "GC-SD: init <-\n");
                break;
            default:
                pprintf_align(&status_paper, RIGHT, "GC-SD: wtf <-\n");
                break;
        }
    }
    if (stage2_opts.bootpatch)
        pprintf_align(&status_paper, RIGHT, "patch BOOT <-\n");
}

static int (*eex_load_psp2bootconfig)(uint32_t myaddr, int* uids, int count, int osloc, int unk) = NULL;
static void stage2_set_ckldr(void *addr) {
    if (!eex_load_psp2bootconfig)
        eex_load_psp2bootconfig = (void*)(*(volatile uint32_t *)NSKBL_PSP2BCFG_STRING_PTR);
    *(volatile uint32_t *)NSKBL_PSP2BCFG_STRING_PTR = (uint32_t)addr;
    nskbl_clean_dcache((void *)NSKBL_PSP2BCFG_STRING_PTR_CACHER, 0x20);
    ILOG("Installed psp2bootconfig hook\n");
}

static int stage2_load_psp2bootconfig_patched(uint32_t myaddr, int* uids, int count, int osloc, int unk) {
    if (stage2_opts.stage3_recovery) {
        DLOG("stage2_hook: entered stage 3 recovery\n");
        main(3);  // Call main with stage 3
    }
    if (stage2_opts.vanilla_boot) {
        DLOG("stage2_hook: continuing vanilla boot\n");
        myaddr = (uint32_t)NSKBL_PSP2BCFG_STRING;
        return nskbl_module_load_direct((void *)&myaddr, uids, count, osloc, unk);
    }
    return eex_load_psp2bootconfig(myaddr, uids, count, osloc, unk);
}

int stage2_apply_config(void) {
    int os0_init_mbroff = stage2_opts.recovery_mbr ? E2X_RECOVERY_MBR_OFFSET : ENSO_EMUMBR_OFFSET;
    unsigned int *os0_master_ctx = (stage2_opts.gcsd_mode == STAGE2_GCSD_MODE_OS0) ? (unsigned int *)NSKBL_DEVICE_GCSD_CTX : (unsigned int *)NSKBL_DEVICE_EMMC_CTX;
    int os0_init_is_scembr = 1;
    if (stage2_opts.gcsd_mode) {
        int sd_mode = (stage2_opts.gcsd_mode == STAGE2_GCSD_MODE_INIT) ? 0 : stage2_opts.gcsd_mode;
        if (sd_mode == STAGE2_GCSD_MODE_OS0)
            sd_mode = STAGE2_GCSD_MODE_INIT; // will mount as os0 later
        ULOG("Initializing GC-SD...\n");
        sd_mode = sd_init(3, sd_mode);
        if (sd_mode < 0) {
            ULOG("ERROR: Failed to initialize GC-SD: %08X\n", sd_mode);
            return MENU_RET_CONTINUE;  // continue without deinit
        }
        if ((stor_init_master(MOUNT_MASTER_GCSD) == MOUNT_MASTER_TYPE_FAT) && (stage2_opts.gcsd_mode == STAGE2_GCSD_MODE_OS0))
            os0_init_is_scembr = 0;  // GC-SD is FAT, not SCE formatted
    }
    if (stage2_opts.vanilla_boot && !eex_load_psp2bootconfig) {
        stage2_set_ckldr((void *)stage2_load_psp2bootconfig_patched);
        ULOG("Enabled hook opt for vanilla boot\n");
    }
    if (stage2_opts.protect_bootarea) {
        g_disable_bootarea_update = 1;  // protect boot area from writes using this recovery
        DACR_OFF(*(g_eex_ports.protect_boot) = 1);  // protect boot area from writes using sdif
        ULOG("Boot area protection enabled\n");
    } else {
        g_disable_bootarea_update = 0;
        DACR_OFF(*(g_eex_ports.protect_boot) = 0);
        ULOG("Boot area protection disabled\n");
    }

    if (g_eex_ports.init_os0(os0_init_mbroff, os0_master_ctx, os0_init_is_scembr) < 0) {
        ULOG("ERROR: Failed to initialize os0 with MBR offset %d, ctx %p, is_scembr %d\n", os0_init_mbroff, os0_master_ctx, os0_init_is_scembr);
        return MENU_RET_CONTINUE;  // continue without deinit
    }

    if (stage2_opts.bootpatch) {
        view_switch(VIEW_DEFAULT);
        if (fmgr_get_nskbl_os0(true) != (uint32_t)-1) {
            ULOG("cmdh_lxp(%s) returned: 0x%08X\n", BOOTPATCH_TXTCFG, cmdh_lxp(BOOTPATCH_TXTCFG));
            fmgr_mount(false, FMGR_OS0_MOUNT);
        } else
            ULOG("ERROR: Failed to mount os0 for boot patching\n");
        view_switch(VIEW_MENU);
    }

    if (stage2_opts.stage3_recovery) {
        if (!eex_load_psp2bootconfig) {
            stage2_set_ckldr((void *)stage2_load_psp2bootconfig_patched);
            ILOG("Enabled hook opt for stage 3 recovery\n");
        }
        ULOG("Entering stage 3 recovery...\n");
        return MENU_RET_FINISH;  // exit without deinit
    }
    return MENU_RET_FINISH_DEINIT;  // deinit & exit
}

static int stage2_save_config(void) {
    ULOG("Preparing the eMMC\n");
    enum MOUNT_MASTER_TYPES mm = stor_init_master(MOUNT_MASTER_EMMC);
    if (mm == MOUNT_MASTER_TYPE_SCE) {
        uint8_t *buf = my_malloc(E2X_RCONF_SIZE);
        if (buf) {
            int ret = stor_read_master(MOUNT_MASTER_EMMC, E2X_RCONF_OFFSET, buf, (E2X_RCONF_SIZE / SECTOR_SIZE));
            if (ret >= 0) {
                memcpy(buf + 0x8, &stage2_opts, sizeof(struct stage2_options));
                ret = stor_write_master(MOUNT_MASTER_EMMC, E2X_RCONF_OFFSET, buf, (E2X_RCONF_SIZE / SECTOR_SIZE));
                if (ret >= 0) {
                    ULOG("New rconfig has been written to eMMC!\n");
                    return stage2_apply_config();
                }else
                    ULOG("Could not write rconfig to eMMC! 0x%08X\n", ret);
            } else
                ULOG("Could not read eMMC: 0x%08X\n", ret);
            my_free(buf);
        } else
            ULOG("Could not malloc for rconf buf\n");
    } else
        ULOG("Could not initialize eMMC: %d!\n", mm);
    return MENU_RET_CONTINUE;
}

int stage2_menu(int selection) {
    int ret = 0;
    if (selection < 0) {  // initial draw
        paper_clear(stage2_menu_s.paper, stage2_menu_s.paper->color);
        pen_reset(stage2_menu_s.paper, stage2_menu_s.paper->pen.color);
        pprintf(stage2_menu_s.paper, "1. Enter stage 3 recovery\n");
        pprintf(stage2_menu_s.paper, "2. Use the recovery MBR\n");
        pprintf(stage2_menu_s.paper, "3. Vanilla boot\n");
        pprintf(stage2_menu_s.paper, "4. Block writes to boot sectors\n");
        pprintf(stage2_menu_s.paper, "5. Change GC-SD mode for nskbl\n");
        pprintf(stage2_menu_s.paper, "6. Run the boot-patching script\n");
        stage2_status_update();
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_CROSS) || BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_SQUARE)) {
        switch (selection) {
            case 0:                        // Enter stage 3 recovery
                stage2_opts.stage3_recovery = !stage2_opts.stage3_recovery;
                if (stage2_opts.stage3_recovery)
                    ULOG("Will enter stage 3 recovery\n");
                else
                    ULOG("Will not enter stage 3 recovery\n");
                stage2_status_update();
                break;
            case 1:  // Use recovery MBR
                stage2_opts.recovery_mbr = !stage2_opts.recovery_mbr;
                if (stage2_opts.recovery_mbr)
                    ULOG("Will boot using recovery emuMBR\n");
                else
                    ULOG("Will boot using default emuMBR\n");
                stage2_status_update();
                break;
            case 2:  // Vanilla boot
                stage2_opts.vanilla_boot = !stage2_opts.vanilla_boot;
                if (stage2_opts.vanilla_boot)
                    ULOG("Will not use the custom kernel loader\n");
                else
                    ULOG("Will use the custom kernel loader\n");
                stage2_status_update();
                break;
            case 3:  // Block boot sector writes
                stage2_opts.protect_bootarea = !stage2_opts.protect_bootarea;
                if (stage2_opts.protect_bootarea)
                    ULOG("Will protect boot area from writes\n");
                else
                    ULOG("Will allow boot area writes\n");
                stage2_status_update();
                break;
            case 4:  // Initialize gc-sd
                stage2_opts.gcsd_mode++;
                if (stage2_opts.gcsd_mode > STAGE2_GCSD_MODE_INIT)
                    stage2_opts.gcsd_mode = STAGE2_GCSD_MODE_DISABLED;
                DLOG("GC-SD init flag set to %d\n", stage2_opts.gcsd_mode);
                stage2_status_update();
                break;
            case 5:
                stage2_opts.bootpatch = !stage2_opts.bootpatch;
                if (stage2_opts.bootpatch)
                    ULOG("Will run %s\n", BOOTPATCH_TXTCFG);
                else
                    ULOG("Will NOT run %s\n", BOOTPATCH_TXTCFG);
                stage2_status_update();
                break;
            default:
                ELOG("Invalid selection %d\n", selection);
                break;
        }
        if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_SQUARE))
            return stage2_apply_config();
        else
            return MENU_RET_CONTINUE;
    } else if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_START))
        return stage2_apply_config();  // apply config and exit
    else if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_SELECT) && g_eex_params.boot_mode == BOOTSTRAP_MODE_EMMC)
        return stage2_save_config();  // save, apply and exit
    return MENU_RET_CONTINUE;  // continue the loop
}


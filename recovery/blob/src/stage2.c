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

struct menu_s stage2_menu_s = {
	.draw = stage2_menu,
	.select = stage2_menu,
	.entry_count = 5,
	.selection = 0,
	.exp_buttons = (CTRL_CROSS | CTRL_SQUARE | CTRL_START),
    .prs_buttons = 0,
    .paper = &menu_paper,
    .selector_color = MENU_SELECTOR_COLR
};

struct stage2_options {
    int recovery_mbr;  // Use recovery MBR
    int stage3_recovery;  // Enter stage 3 recovery
    int vanilla_boot;  // Boot vanilla OS
    int protect_bootarea;  // Protect boot area from writes
    int gcsd_mode;  // 0: disable, 1: sd0, 2: os0, 3: init
    int update_emmc_recovery;  // Update EMMC recovery from GC-SD
} stage2_opts = {
    .recovery_mbr = 0,
    .stage3_recovery = 0,
    .vanilla_boot = 0,
    .protect_bootarea = 0,
    .gcsd_mode = STAGE2_GCSD_MODE_DISABLED,
    .update_emmc_recovery = 0
};

static void stage2_status_update(void) {
    paper_clear(&status_paper, MENU_PAPER_COLR);
    pen_reset(&status_paper, STATUS_PEN_COLR);
    pprintf_align(&status_paper, LEFT, "S2\n");
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
    if (stage2_opts.update_emmc_recovery)
        pprintf_align(&status_paper, RIGHT, "update EMMC R <-\n");
}

static int (*eex_load_psp2bootconfig)(uint32_t myaddr, int* uids, int count, int osloc, int unk) = NULL;
static void stage2_set_ckldr(void *addr) {
    if (!eex_load_psp2bootconfig)
        eex_load_psp2bootconfig = (void*)(*(volatile uint32_t *)NSKBL_PSP2BCFG_STRING_PTR);
    *(volatile uint32_t *)NSKBL_PSP2BCFG_STRING_PTR = (uint32_t)addr;
    nskbl_clean_dcache((void *)NSKBL_PSP2BCFG_STRING_PTR_CACHER, 0x20);
    LOG("Installed psp2bootconfig hook\n");
}

static int stage2_load_psp2bootconfig_patched(uint32_t myaddr, int* uids, int count, int osloc, int unk) {
    if (stage2_opts.stage3_recovery) {
        LOG("stage2_hook: entered stage 3 recovery\n");
        main(3);  // Call main with stage 3
    }
    if (stage2_opts.vanilla_boot) {
        LOG("stage2_hook: continuing vanilla boot\n");
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
        alllog("Initializing GC-SD...\n");
        sd_mode = sd_init(3, sd_mode);
        if (sd_mode < 0) {
            alllog("ERROR: Failed to initialize GC-SD: %08X\n", sd_mode);
            return MENU_RET_CONTINUE;  // continue without deinit
        }
        if ((stor_init_master(MOUNT_MASTER_GCSD) == MOUNT_MASTER_TYPE_FAT) && (stage2_opts.gcsd_mode == STAGE2_GCSD_MODE_OS0))
            os0_init_is_scembr = 0;  // GC-SD is FAT, not SCE formatted
    }
    if (stage2_opts.vanilla_boot && !eex_load_psp2bootconfig) {
        stage2_set_ckldr((void *)stage2_load_psp2bootconfig_patched);
        LOG("Enabled hook opt for vanilla boot\n");
    }
    if (stage2_opts.protect_bootarea) {
        g_disable_bootarea_update = 1;  // protect boot area from writes using this recovery
        DACR_OFF(*(g_eex_ports.protect_boot) = 1);  // protect boot area from writes using sdif
        LOG("Boot area protection enabled\n");
    } else {
        g_disable_bootarea_update = 0;
        DACR_OFF(*(g_eex_ports.protect_boot) = 0);
        LOG("Boot area protection disabled\n");
    }

    if (g_eex_ports.init_os0(os0_init_mbroff, os0_master_ctx, os0_init_is_scembr) < 0) {
        alllog("ERROR: Failed to initialize os0 with MBR offset %d, ctx %p, is_scembr %d\n", os0_init_mbroff, os0_master_ctx, os0_init_is_scembr);
        return MENU_RET_CONTINUE;  // continue without deinit
    }

    if (stage2_opts.stage3_recovery) {
        if (!eex_load_psp2bootconfig) {
            stage2_set_ckldr((void *)stage2_load_psp2bootconfig_patched);
            LOG("Enabled hook opt for stage 3 recovery\n");
        }
        alllog("Entering stage 3 recovery...\n");
        return MENU_RET_FINISH;  // exit without deinit
    }
    return MENU_RET_FINISH_DEINIT;  // deinit & exit
}

int stage2_menu(int selection) {
    int ret = 0;
    if (selection < 0) {  // initial draw
        paper_clear(stage2_menu_s.paper, stage2_menu_s.paper->color);
        pen_reset(stage2_menu_s.paper, stage2_menu_s.paper->pen.color);
        pprintf(stage2_menu_s.paper, "1. Enter stage 3 recovery\n");
        pprintf(stage2_menu_s.paper, "2. Use recovery MBR\n");
        pprintf(stage2_menu_s.paper, "3. Vanilla boot\n");
        pprintf(stage2_menu_s.paper, "4. Block writes to boot sectors\n");
        pprintf(stage2_menu_s.paper, "5. Change GC-SD mode for nskbl\n");
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_CROSS) || BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_SQUARE)) {
        switch (selection) {
            case 0:                        // Enter stage 3 recovery
                stage2_opts.stage3_recovery = !stage2_opts.stage3_recovery;
                if (stage2_opts.stage3_recovery)
                    alllog("Will enter stage 3 recovery\n");
                else
                    alllog("Will not enter stage 3 recovery\n");
                stage2_status_update();
                break;
            case 1:  // Use recovery MBR
                stage2_opts.recovery_mbr = !stage2_opts.recovery_mbr;
                if (stage2_opts.recovery_mbr)
                    alllog("Will boot using recovery emuMBR\n");
                else
                    alllog("Will boot using default emuMBR\n");
                stage2_status_update();
                break;
            case 2:  // Vanilla boot
                stage2_opts.vanilla_boot = !stage2_opts.vanilla_boot;
                if (stage2_opts.vanilla_boot)
                    alllog("Will not use the custom kernel loader\n");
                else
                    alllog("Will use the custom kernel loader\n");
                stage2_status_update();
                break;
            case 3:  // Block boot sector writes
                stage2_opts.protect_bootarea = !stage2_opts.protect_bootarea;
                if (stage2_opts.protect_bootarea)
                    alllog("Will protect boot area from writes\n");
                else
                    alllog("Will allow boot area writes\n");
                stage2_status_update();
                break;
            case 4:  // Initialize gc-sd
                stage2_opts.gcsd_mode++;
                if (stage2_opts.gcsd_mode > STAGE2_GCSD_MODE_INIT)
                    stage2_opts.gcsd_mode = STAGE2_GCSD_MODE_DISABLED;
                LOG("GC-SD init flag set to %d\n", stage2_opts.gcsd_mode);
                stage2_status_update();
                break;
            default:
                LOG("Invalid selection %d\n", selection);
                break;
        }
        if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_SQUARE))
            return stage2_apply_config();
        else
            return MENU_RET_CONTINUE;
    } else if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_START))
        return stage2_apply_config();  // apply config and exit
    return MENU_RET_CONTINUE;  // continue the loop
}


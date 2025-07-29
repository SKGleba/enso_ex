#include <baremetal/cdram.h>
#include <baremetal/ctrl.h>
#include <baremetal/display.h>
#include <baremetal/draw.h>
#include <baremetal/font.h>
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

#include "../../../../../core/enso.h"
#include "../../../../../core/ex_defs.h"
#include "bm_ext.h"
#include "main.h"
#include "nskbl.h"
#include "paper.h"
#include "stage2.h"
#include "stor.h"

struct menu_s stage2_menu_s = {
	.draw = stage2_menu,
	.select = stage2_menu,
	.entry_count = 5,
	.selection = 0,
	.exp_buttons = CTRL_CROSS,
    .prs_buttons = 0,
    .paper = &menu_paper,
    .selector_color = MENU_SELECTOR_COLR
};

struct stage2_options {
    int recovery_mbr;  // Use recovery MBR
    int stage3_recovery;  // Enter stage 3 recovery
    int vanilla_boot;  // Boot vanilla OS
    int gcsd_mode;  // 0: disable, 1: sd0, 2: os0, 3: init
} stage2_opts = {
    .recovery_mbr = 0,
    .stage3_recovery = 0,
    .vanilla_boot = 0,
    .gcsd_mode = STAGE2_GCSD_MODE_DISABLED
};

static void stage2_status_update(void) {
    paper_clear(status_paper, MENU_PAPER_COLR);
    pen_reset(status_pen, STATUS_PEN_COLR);
    pprintf_align(&status_paper, LEFT, "S2\n");
    if (stage2_opts.recovery_mbr)
        pprintf_align(&status_paper, RIGHT, "use R MBR <-\n");
    if (stage2_opts.stage3_recovery)
        pprintf_align(&status_paper, RIGHT, "goto s3 <-\n");
    if (stage2_opts.vanilla_boot)
        pprintf_align(&status_paper, RIGHT, "vanilla boot <-\n");
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
    
}

static int (*eex_load_psp2bootconfig)(uint32_t myaddr, int* uids, int count, int osloc, int unk) = NULL;
static void stage2_set_ckldr(void *addr) {
    if (!eex_load_psp2bootconfig)
        eex_load_psp2bootconfig = (void*)(*(volatile uint32_t *)NSKBL_PSP2BCFG_STRING_PTR);
    *(volatile uint32_t *)NSKBL_PSP2BCFG_STRING_PTR = (uint32_t)addr;
    nskbl_clean_dcache((void *)NSKBL_PSP2BCFG_STRING_PTR_CACHER, 0x20);
    scrprintf("Installed psp2bootconfig hook\n");
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

int stage2_menu(int selection) {
    int ret = 0;
    if (selection < 0) {  // initial draw
        ppaper_clear(stage2_menu_s.paper, stage2_menu_s.paper->color);
        ppen_reset(stage2_menu_s.paper->pen, stage2_menu_s.paper->pen->color);
        pprintf(stage2_menu_s.paper, "1. Continue boot\n");
        pprintf(stage2_menu_s.paper, "2. Use recovery MBR\n");
        pprintf(stage2_menu_s.paper, "3. Enter stage 3 recovery\n");
        pprintf(stage2_menu_s.paper, "4. Vanilla boot\n");
        pprintf(stage2_menu_s.paper, "5. Change GC-SD mode for nskbl\n");
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_CROSS)) {
        switch (selection) {
            case 0:  // Continue boot
                if (stage2_opts.recovery_mbr)
                    eex_init_os0(E2X_RECOVERY_MBR_OFFSET);
                else
                    eex_init_os0(ENSO_EMUMBR_OFFSET);
                if (stage2_opts.gcsd_mode) {
                    scrprintf("Initializing GC-SD...\n");
                    scrprintf("sd0 init %s\n", sd_init(3, (stage2_opts.gcsd_mode == STAGE2_GCSD_MODE_INIT) ? 0 : stage2_opts.gcsd_mode) ? "failed" : "OK");
                }
                if (stage2_opts.stage3_recovery) {
                    scrprintf("Entering stage 3 recovery...\n");
                    return MENU_RET_FINISH;  // exit without deinit
                }
                return MENU_RET_FINISH_DEINIT;  // deinit & exit
            case 1:                             // Use recovery MBR
                stage2_opts.recovery_mbr = !stage2_opts.recovery_mbr;
                if (stage2_opts.recovery_mbr) {
                    scrprintf("Will boot using recovery emuMBR\n");
                } else {
                    scrprintf("Will boot using default emuMBR\n");
                }
                stage2_status_update();
                return MENU_RET_CONTINUE;  // continue the loop
            case 2:                        // Enter stage 3 recovery
                stage2_opts.stage3_recovery = !stage2_opts.stage3_recovery;
                if (stage2_opts.stage3_recovery) {
                    if (!eex_load_psp2bootconfig)
                        stage2_set_ckldr((void *)stage2_load_psp2bootconfig_patched);
                    scrprintf("Enabled hook opt for stage 3 recovery\n");
                } else {
                    scrprintf("Disabled hook opt for stage 3 recovery\n");
                }
                stage2_status_update();
                return MENU_RET_CONTINUE;
            case 3:  // Vanilla boot
                stage2_opts.vanilla_boot = !stage2_opts.vanilla_boot;
                if (stage2_opts.vanilla_boot) {
                    if (!eex_load_psp2bootconfig)
                        stage2_set_ckldr((void *)stage2_load_psp2bootconfig_patched);
                    scrprintf("Enabled hook opt for vanilla boot\n");
                } else {
                    scrprintf("Disabled hook opt for vanilla boot\n");
                }
                stage2_status_update();
                return MENU_RET_CONTINUE;
            case 4:  // Initialize gc-sd
                stage2_opts.gcsd_mode++;
                if (stage2_opts.gcsd_mode > STAGE2_GCSD_MODE_INIT)
                    stage2_opts.gcsd_mode = STAGE2_GCSD_MODE_DISABLED;
                scrprintf("GC-SD init flag set to %d\n", stage2_opts.gcsd_mode);
                stage2_status_update();
                return MENU_RET_CONTINUE;
            default:
                scrprintf("Invalid selection %d\n", selection);
                return MENU_RET_CONTINUE;
        }
    }
    return MENU_RET_CONTINUE;  // continue the loop
}


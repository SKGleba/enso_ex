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
	.exp_buttons = (CTRL_CROSS | CTRL_SQUARE),
    .prs_buttons = 0,
    .paper = &menu_paper,
    .selector_color = MENU_SELECTOR_COLR
};

struct stage2_options {
    int recovery_mbr;  // Use recovery MBR
    int stage3_recovery;  // Enter stage 3 recovery
    int vanilla_boot;  // Boot vanilla OS
    int gcsd_mode;  // 0: disable, 1: sd0, 2: os0, 3: init
    int update_emmc_recovery;  // Update EMMC recovery from GC-SD
} stage2_opts = {
    .recovery_mbr = 0,
    .stage3_recovery = 0,
    .vanilla_boot = 0,
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

static int stage2_update_emmc_recovery(void) {
    alllog("Updating EMMC recovery from GC-SD...\n");
    int memblock_id = sceKernelAllocMemBlock("EMMC_RECOVERY", MEMBLOCK_TYPE_RW, EEX_RBLOB_SIZE_SECTORS * SECTOR_SIZE, NULL);
    if (memblock_id < 0) {
        LOG("ERROR: Failed to allocate memory block for EMMC recovery\n");
        return -1;
    }
    void *memblock_va = NULL;
    if ((sceKernelGetMemBlockBase(memblock_id, &memblock_va) < 0) || !memblock_va) {
        LOG("ERROR: Failed to get memory block base for EMMC recovery\n");
        sceKernelFreeMemBlock(memblock_id);
        return -1;
    }
    {
        LOG("Reading recovery blob from GC-SD...\n");
        memset(memblock_va, 0, EEX_RBLOB_SIZE_SECTORS * SECTOR_SIZE);
        int ret = SDREAD(EEX_RBLOB_SECTOR_GCSD, memblock_va, EEX_RBLOB_SIZE_SECTORS);
        if (ret < 0) {
            LOG("ERROR: Failed to read recovery blob from GC-SD: %08X\n", ret);
            sceKernelFreeMemBlock(memblock_id);
            return -1;
        }
        int prev = g_disable_bootarea_update;
        g_disable_bootarea_update = 0;  // allow bootarea update
        LOG("Writing recovery blob to EMMC...\n");
        ret = MMCWRITE(EEX_RBLOB_SECTOR_EMMC, memblock_va, EEX_RBLOB_SIZE_SECTORS);
        g_disable_bootarea_update = prev;  // restore previous state
        if (ret < 0) {
            LOG("ERROR: Failed to write recovery blob to EMMC: %08X\n", ret);
            sceKernelFreeMemBlock(memblock_id);
            return -1;
        }
    }
    {
        LOG("Reading recovery config from GC-SD...\n");
        memset(memblock_va, 0, EEX_RCONFIG_SIZE_SECTORS * SECTOR_SIZE);
        int ret = SDREAD(EEX_RCONFIG_SECTOR_GCSD, memblock_va, EEX_RCONFIG_SIZE_SECTORS);
        if (ret < 0) {
            LOG("ERROR: Failed to read recovery config from GC-SD: %08X\n", ret);
            sceKernelFreeMemBlock(memblock_id);
            return -1;
        }
        int prev = g_disable_bootarea_update;
        g_disable_bootarea_update = 0;  // allow bootarea update
        LOG("Writing recovery config to EMMC...\n");
        ret = MMCWRITE(EEX_RCONFIG_SECTOR_EMMC, memblock_va, EEX_RCONFIG_SIZE_SECTORS);
        g_disable_bootarea_update = prev;  // restore previous state
        if (ret < 0) {
            LOG("ERROR: Failed to write recovery config to EMMC: %08X\n", ret);
            sceKernelFreeMemBlock(memblock_id);
            return -1;
        }
    }
    sceKernelFreeMemBlock(memblock_id);
    alllog("EMMC recovery updated successfully\n");
    return 0;
}

int stage2_apply_config(void) {
    if (g_eex_params.init_os0) { // EMMC mode
        LOG("Applying stage 2 config [EMMC mode]\n");
        if (stage2_opts.recovery_mbr)
            g_eex_params.init_os0(E2X_RECOVERY_MBR_OFFSET);
        else
            g_eex_params.init_os0(ENSO_EMUMBR_OFFSET);

        if (stage2_opts.gcsd_mode) {
            alllog("Initializing GC-SD...\n");
            alllog("sd0 init %s\n", sd_init(3, (stage2_opts.gcsd_mode == STAGE2_GCSD_MODE_INIT) ? 0 : stage2_opts.gcsd_mode) ? "failed" : "OK");
        }
    } else {
        LOG("Applying stage 2 config [GC-SD mode]\n");
        if (stage2_opts.update_emmc_recovery && ((stage2_update_emmc_recovery() < 0))) {
            alllog("ERROR: Failed to update EMMC recovery from GC-SD!\n");
            alllog("Aborting stage 2 config application\n");
            return MENU_RET_CONTINUE;
        }
    }
    if (stage2_opts.vanilla_boot && !eex_load_psp2bootconfig) {
        stage2_set_ckldr((void *)stage2_load_psp2bootconfig_patched);
        LOG("Enabled hook opt for vanilla boot\n");
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
        pprintf(stage2_menu_s.paper, "1. Continue boot\n");
        pprintf(stage2_menu_s.paper, "2. Enter stage 3 recovery\n");
        pprintf(stage2_menu_s.paper, "3. Vanilla boot\n");
        if (g_eex_params.init_os0) { // EMMC mode
            stage2_menu_s.entry_count = 5;
            pprintf(stage2_menu_s.paper, "4. Use recovery MBR\n");
            pprintf(stage2_menu_s.paper, "5. Change GC-SD mode for nskbl\n");
        } else {
            stage2_menu_s.entry_count = 4;
            pprintf(stage2_menu_s.paper, "4. Update EMMC recovery from GC-SD\n");
        }
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_CROSS) || BMX_CTRL_BUTTON_HELD(stage2_menu_s.prs_buttons, CTRL_SQUARE)) {
        switch (selection) {
            case 0:  // Continue boot
                return stage2_apply_config();  // apply config and exit
            case 1:                        // Enter stage 3 recovery
                stage2_opts.stage3_recovery = !stage2_opts.stage3_recovery;
                if (stage2_opts.stage3_recovery)
                    alllog("Will enter stage 3 recovery\n");
                else
                    alllog("Will not enter stage 3 recovery\n");
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
            case 3:  // Use recovery MBR
                if (g_eex_params.init_os0) { // EMMC mode
                    stage2_opts.recovery_mbr = !stage2_opts.recovery_mbr;
                    if (stage2_opts.recovery_mbr)
                        alllog("Will boot using recovery emuMBR\n");
                    else
                        alllog("Will boot using default emuMBR\n");
                } else {
                    stage2_opts.update_emmc_recovery = !stage2_opts.update_emmc_recovery;
                    if (stage2_opts.update_emmc_recovery)
                        alllog("Will update EMMC recovery from GC-SD\n");
                    else
                        alllog("Will not update EMMC recovery from GC-SD\n");
                }
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
    }
    return MENU_RET_CONTINUE;  // continue the loop
}


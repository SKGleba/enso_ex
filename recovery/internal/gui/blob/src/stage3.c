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
#include "stage3.h"
#include "stor.h"

#include "ff.h"

struct menu_s stage3_menu_s = {.draw = stage3_menu,
                               .select = stage3_menu,
                               .entry_count = 3,
                               .selection = 0,
                               .exp_buttons = CTRL_CROSS,
                               .prs_buttons = 0,
                               .paper = &menu_paper,
                               .selector_color = MENU_SELECTOR_COLR};

static void try_init_mounts(void) {
    int ret;
    scrprintf("Initializing masters...\n");
    ret = stor_init_master(MOUNT_MASTER_EMMC);
    if (ret < 0) {
        scrprintf("Failed to init eMMC: %d\n", ret);
    }
    ret = stor_init_master(MOUNT_MASTER_GCSD);
    if (ret < 0) {
        scrprintf("Failed to init GCSD: %d\n", ret);
    }

    scrprintf("Initializing mounts...\n");
    ret = stor_init_mount(0, MOUNT_MASTER_EMMC, 3);
    if (ret < 0) {
        scrprintf("Failed to init mount 0: %d\n", ret);
    }
    ret = stor_init_mount(1, MOUNT_MASTER_GCSD, 0);
    if (ret < 0) {
        scrprintf("Failed to init mount 1: %d\n", ret);
    }

    uint8_t data[SECTOR_SIZE];
    scrprintf("Reading from mount 0...\n");
    ret = stor_read_mount(0, 0, data, 1);
    if (ret < 0) {
        scrprintf("Failed to read from mount 0: %d\n", ret);
    } else {
        scrprintf("Mount 0 data: %02X %02X %02X %02X\n", data[0], data[1], data[2], data[3]);
    }
    scrprintf("Reading from mount 1...\n");
    ret = stor_read_mount(1, 0, data, 1);
    if (ret < 0) {
        scrprintf("Failed to read from mount 1: %d\n", ret);
    } else {
        scrprintf("Mount 1 data: %02X %02X %02X %02X\n", data[0], data[1], data[2], data[3]);
    }
}

static void try_ff_mounts(void) {
    FRESULT ret;
    scrprintf("Initializing FF mounts...\n");
    FATFS fs;
    UINT br;
    ret = f_mount(&fs, "mnt0:", 1);
    if (ret != FR_OK) {
        scrprintf("Failed to mount mnt0: %d\n", ret);
        return;
    } else {
        scrprintf("Mounted mnt0 successfully.\n");
    }
    FIL file;
    ret = f_open(&file, "mnt0:/psp2bootconfig.skprx", FA_READ);
    if (ret != FR_OK) {
        scrprintf("Failed to open psp2bootconfig.skprx: %d\n", ret);
        return;
    } else {
        scrprintf("Opened psp2bootconfig.skprx successfully.\n");
    }
    uint8_t buffer[512];
    ret = f_read(&file, buffer, sizeof(buffer), &br);
    if (ret != FR_OK) {
        scrprintf("Failed to read from psp2bootconfig.skprx: %d\n", ret);
    } else {
        scrprintf("Read from psp2bootconfig.skprx successfully.\n");
        scrprintf("Data: %02X %02X %02X %02X\n", buffer[0], buffer[1], buffer[2], buffer[3]);
    }
    ret = f_close(&file);
    if (ret != FR_OK) {
        scrprintf("Failed to close file: %d\n", ret);
    } else {
        scrprintf("Closed file successfully.\n");
    }
}

int stage3_menu(int selection) {
    if (selection < 0) {  // initial draw
        ppaper_clear(stage3_menu_s.paper, stage3_menu_s.paper->color);
        ppen_reset(stage3_menu_s.paper->pen, stage3_menu_s.paper->pen->color);
        pprintf(stage3_menu_s.paper, "1. Continue boot\n");
        pprintf(stage3_menu_s.paper, "2. Init mounts\n");
        pprintf(stage3_menu_s.paper, "3. Try FF volume mounts\n");
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stage3_menu_s.prs_buttons, CTRL_CROSS)) {
        switch (selection) {
            case 0:  // Continue boot
                return MENU_RET_FINISH_DEINIT;  // deinit & exit
            case 1:  // Init mounts
                try_init_mounts();
                return MENU_RET_CONTINUE;  // continue the loop
            case 2:  // Try FF volume mounts
                try_ff_mounts();
                return MENU_RET_CONTINUE;  // continue the loop
            default:
                scrprintf("Invalid selection %d\n", selection);
                return MENU_RET_CONTINUE;  // continue the loop
        }
    }
    return MENU_RET_CONTINUE;  // continue the loop
}

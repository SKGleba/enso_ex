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

#include "ff.h"

struct menu_s stage3_menu_s = {.draw = stage3_menu,
                               .select = stage3_menu,
                               .entry_count = 5,
                               .selection = 0,
                               .exp_buttons = CTRL_CROSS,
                               .prs_buttons = 0,
                               .paper = &menu_paper,
                               .selector_color = MENU_SELECTOR_COLR};

static void try_init_mounts(void) {
    int ret;
    LOG("Initializing masters...\n");
    ret = stor_init_master(MOUNT_MASTER_EMMC);
    if (ret < 0) {
        LOG("Failed to init eMMC: %d\n", ret);
    }
    ret = stor_init_master(MOUNT_MASTER_GCSD);
    if (ret < 0) {
        LOG("Failed to init GCSD: %d\n", ret);
    }

    LOG("Initializing mounts...\n");
    ret = stor_init_mount(0, MOUNT_MASTER_EMMC, 3);
    if (ret < 0) {
        LOG("Failed to init mount 0: %d\n", ret);
    }
    ret = stor_init_mount(1, MOUNT_MASTER_GCSD, 0);
    if (ret < 0) {
        LOG("Failed to init mount 1: %d\n", ret);
    }

    uint8_t data[SECTOR_SIZE];
    LOG("Reading from mount 0...\n");
    ret = stor_read_mount(0, 0, data, 1);
    if (ret < 0) {
        LOG("Failed to read from mount 0: %d\n", ret);
    } else {
        LOG("Mount 0 data: %02X %02X %02X %02X\n", data[0], data[1], data[2], data[3]);
    }
    LOG("Reading from mount 1...\n");
    ret = stor_read_mount(1, 0, data, 1);
    if (ret < 0) {
        LOG("Failed to read from mount 1: %d\n", ret);
    } else {
        LOG("Mount 1 data: %02X %02X %02X %02X\n", data[0], data[1], data[2], data[3]);
    }
}

static void try_ff_mounts(void) {
    FRESULT ret;
    LOG("Initializing FF mounts...\n");
    FATFS fs;
    UINT br;
    ret = f_mount(&fs, "mnt0:", 1);
    if (ret != FR_OK) {
        LOG("Failed to mount mnt0: %d\n", ret);
        return;
    } else {
        LOG("Mounted mnt0 successfully.\n");
    }
    FIL file;
    ret = f_open(&file, "mnt0:/psp2bootconfig.skprx", FA_READ);
    if (ret != FR_OK) {
        LOG("Failed to open psp2bootconfig.skprx: %d\n", ret);
        return;
    } else {
        LOG("Opened psp2bootconfig.skprx successfully.\n");
    }
    uint8_t buffer[512];
    ret = f_read(&file, buffer, sizeof(buffer), &br);
    if (ret != FR_OK) {
        LOG("Failed to read from psp2bootconfig.skprx: %d\n", ret);
    } else {
        LOG("Read from psp2bootconfig.skprx successfully.\n");
        LOG("Data: %02X %02X %02X %02X\n", buffer[0], buffer[1], buffer[2], buffer[3]);
    }
    ret = f_close(&file);
    if (ret != FR_OK) {
        LOG("Failed to close file: %d\n", ret);
    } else {
        LOG("Closed file successfully.\n");
    }
}

static void try_file_create(void) {
    FRESULT ret;
    LOG("Initializing FF mounts...\n");
    FATFS fs;
    UINT br;
    ret = f_mount(&fs, "mnt1:", 1);
    if (ret != FR_OK) {
        LOG("Failed to mount mnt1: %d\n", ret);
        return;
    } else {
        LOG("Mounted mnt1 successfully.\n");
    }
    FIL file;
    ret = f_open(&file, "mnt1:/testfile.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (ret != FR_OK) {
        LOG("Failed to create testfile.txt: %d\n", ret);
        return;
    } else {
        LOG("Created testfile.txt successfully.\n");
    }
    const char *data = "Hello, this is a test file.";
    UINT bw;
    ret = f_write(&file, data, strlen(data), &bw);
    if (ret != FR_OK) {
        LOG("Failed to write to testfile.txt: %d\n", ret);
    } else {
        LOG("Wrote %u bytes to testfile.txt successfully.\n", bw);
    }
    ret = f_close(&file);
    if (ret != FR_OK) {
        LOG("Failed to close file: %d\n", ret);
    } else {
        LOG("Closed file successfully.\n");
    }
}

int stage3_menu(int selection) {
    if (selection < 0) {  // initial draw
        paper_clear(stage3_menu_s.paper, stage3_menu_s.paper->color);
        pen_reset(stage3_menu_s.paper->pen, stage3_menu_s.paper->pen->color);
        pprintf(stage3_menu_s.paper, "1. Continue boot\n");
        pprintf(stage3_menu_s.paper, "2. Init mounts\n");
        pprintf(stage3_menu_s.paper, "3. Try FF volume mounts\n");
        pprintf(stage3_menu_s.paper, "4. Get current time\n");
        pprintf(stage3_menu_s.paper, "5. Try file create\n");
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
            case 3:  // Get current time
                LOG("Current time: %08X\n", bmx_get_time(NULL));
                return MENU_RET_CONTINUE;  // continue the loop
            case 4:  // Try file create
                try_file_create();
                return MENU_RET_CONTINUE;  // continue the loop
            default:
                LOG("Invalid selection %d\n", selection);
                return MENU_RET_CONTINUE;  // continue the loop
        }
    }
    return MENU_RET_CONTINUE;  // continue the loop
}

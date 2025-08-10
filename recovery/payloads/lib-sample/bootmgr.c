
#include <inttypes.h>
#include <stddef.h>

#include "bootmgr.h"

static int init_logview(bool init) {  // initialize display and exr temp view for visual log
    if (init) {
        if (g_eex_params.boot_mode == BOOTSTRAP_MODE_LIB) {
            LOG("Enabling display & views\n");
            if (bmx_displaymgr(DISPLAYMGR_NSTATE_ON, DISPLAYMGR_OPT_INCLUDE_FB) < 0) {
                LOG("Failed to initialize display!\n");
                return -1;
            }
            if (view_init() < 0) {
                LOG("Failed to initialize views!\n");
                return -1;
            }
        }
        view_switch(VIEW_TEMP);
        LOG("Initializing the default view..\n");
        default_paper.view_idx = VIEW_TEMP;
        // draw the title area
        paper_area(&default_paper, DFL_PAPER_START_X, DFL_PAPER_START_Y, DFL_PAPER_END_X, DFL_PAPER_END_Y);
        paper_clear(&default_paper, CYAN);  // invert vs normal recovery
        pen_reset(&default_paper, RED);     // ^
        pprintf_align(&default_paper, CENTER, "enso_ex recovery LIB payload example\nby skgleba\n");
        // draw the log area
        paper_area(&default_paper, LOG_PAPER_START_X, LOG_PAPER_START_Y, LOG_PAPER_END_X, LOG_PAPER_END_Y);
        paper_clear(&default_paper, LOG_PAPER_COLR);
        pen_reset(&default_paper, LOG_PEN_COLR);
        default_paper.blank_mode = LOG_PAPER_BLANK_MODE;
        g_log_targets |= LOG_TARGET_LOGPAPER;
    } else {
        g_log_targets &= ~LOG_TARGET_LOGPAPER;
        default_paper.view_idx = VIEW_DEFAULT;
        view_switch(VIEW_DEFAULT);
        if (g_eex_params.boot_mode == BOOTSTRAP_MODE_LIB) {
            LOG("Disabling display & views\n");
            bmx_displaymgr(DISPLAYMGR_NSTATE_OFF, DISPLAYMGR_OPT_INCLUDE_FB);
            delay(4000);
        }
    }
    return 0;
}

int b_main(void) {
    if (init_logview(true) < 0)
        return -1;
    LOG("Hello World!\nPress SQUARE to exit\n");
    bmx_ctrl_wait(CTRL_SQUARE, 4000, 1);
    init_logview(false);
    return 0;
}

__attribute__((section(".text.bootstart"))) int bootstart(void) {
	struct eex_param_s tmp_eex_params = {
            .boot_mode = BOOTSTRAP_MODE_LIB, // _BOOTMGR will autoinit the display and require calling deinit() on exit
            .get_hwcfg_patched = (int (*)(patchedHwcfgStruct *))(*(uint32_t*)NSKBL_EXPORTS(NSKBL_EXPORTS_GET_HWCFG_N)),
            .stage2_config = NULL
        };
    if (init(&tmp_eex_params) < 0) {
        nskbl_printf("Failed to initialize e2xr in LIB mode\n");
        return -1;
    }
    b_main();
    return E2X_EXE_RET_NORESIDENT; // indicate that we can be freed
}
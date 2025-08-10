// 0syscall6 for non-secure bootloader

#include <inttypes.h>
#include <stddef.h>

#include "pie.h"

static struct recovery_export_s *_r = NULL;  // rblob exports for PICs
static int init_rxp(void);

static int rxp_prev_log_targets = LOG_TARGET_CONSOLE;
static int init_logview(bool init) { // initialize display and exr temp view for visual log
    if (init) {
        if (r_eex_params->boot_mode == BOOTSTRAP_MODE_LIB) {
            r_LOG("Enabling display & views\n");
            if (r_bmx_displaymgr(DISPLAYMGR_NSTATE_ON, DISPLAYMGR_OPT_INCLUDE_FB) < 0) {
                r_LOG("Failed to initialize display!\n");
                return -1;
            }
            if (r_view_init() < 0) {
                r_LOG("Failed to initialize views!\n");
                return -1;
            }
        } else {
            DACR_OFF(
                rxp_prev_log_targets = r_g_log_targets;
            );
        }
        r_view_switch(VIEW_TEMP);
        r_LOG("Initializing the default view..\n");
        r_default_paper->view_idx = VIEW_TEMP;
        // draw the title area
        paper_area(r_default_paper, DFL_PAPER_START_X, DFL_PAPER_START_Y, DFL_PAPER_END_X, DFL_PAPER_END_Y);
        paper_clear(r_default_paper, CYAN);        // invert vs normal recovery
        pen_reset(r_default_paper, RED); // ^
        pprintf_align(r_default_paper, CENTER, "enso_ex recovery PIC payload example\nby skgleba\n");
        // draw the log area
        paper_area(r_default_paper, LOG_PAPER_START_X, LOG_PAPER_START_Y, LOG_PAPER_END_X, LOG_PAPER_END_Y);
        paper_clear(r_default_paper, LOG_PAPER_COLR);
        pen_reset(r_default_paper, LOG_PEN_COLR);
        r_default_paper->blank_mode = LOG_PAPER_BLANK_MODE;
        r_g_log_targets |= LOG_TARGET_LOGPAPER;
    } else {
        r_g_log_targets = rxp_prev_log_targets;
        r_default_paper->view_idx = VIEW_DEFAULT;
        r_view_switch(VIEW_DEFAULT);
        if (r_eex_params->boot_mode == BOOTSTRAP_MODE_LIB) {
            r_LOG("Disabling display & views\n");
            r_bmx_displaymgr(DISPLAYMGR_NSTATE_OFF, DISPLAYMGR_OPT_INCLUDE_FB);
            _r->lbm->delay(4000);
        }
    }
    return 0;
}

static int main(void) {
    if (init_logview(true) < 0)
        return -1;
    r_LOG("Hello World!\nPress SQUARE to exit\n");
    r_bmx_ctrl_wait(CTRL_SQUARE, 4000, 1);
    init_logview(false);
    return 0;
}

// -- PIC init scripts --
__attribute__((section(".text.start"))) int start(void) {
	if (init_rxp() < 0)
        return E2X_EXE_RET_NORESIDENT;
    r_LOG("Recovery blob initialized successfully\n");
    main();
    return E2X_EXE_RET_NORESIDENT; // indicate that we can be freed
}

// -- Recovery blob init scripts --
static struct recovery_export_s *p_find_rxp(uint32_t blob, bool validate) {
    nskbl_printf("Scanning for recovery blob @ 0x%08X - 0x%08X\n", blob, blob + E2X_RBLOB_SIZE);
    for (uint32_t p = blob; p < blob + E2X_RBLOB_SIZE; p += 4) {
        struct recovery_export_s *prxp = (struct recovery_export_s *)p;
        if ((prxp->magic[0] == EXPORTS_MAGIC_1) && (prxp->magic[1] == EXPORTS_MAGIC_2)) {
            nskbl_printf("Found valid recovery blob exports @ 0x%08X\n", p);
            if (validate) {
                uint8_t *kbl_session_id = (uint8_t *)(ns_kbl_param + SESSION_UID_KBLP_OFF);
                for (int i = 0; i < SESSION_UID_KBLP_SIZE; i++) {
                    if (kbl_session_id[i] != prxp->session_id[i]) {
                        nskbl_printf("Ignoring found blob because of session ID mismatch at byte %d: 0x%02X != 0x%02X\n", i, kbl_session_id[i],
                                     prxp->session_id[i]);
                        return NULL;
                    }
                }
            }
            return prxp;
        }
    }
    return NULL;
}

static int init_rxp(void) {
    int ret = 0;
    struct recovery_export_s *prxp = p_find_rxp(E2X_RBLOB_PADDR, true);
    if (!prxp)
        prxp = p_find_rxp(E2X_BOOTMGR_PADDR, true);
    if (!prxp) {
        nskbl_printf("No recovery blob found, reading eMMC one...\n");
        ret = r_EMMCREAD(E2X_RBLOB_OFFSET, (void*)E2X_RBLOB_PADDR, E2X_RBLOB_SIZE / SECTOR_SIZE);
        if (ret < 0) {
            nskbl_printf("Failed to read recovery blob from eMMC: 0x%08X\n", ret);
            return -1;
        }
        prxp = p_find_rxp(E2X_RBLOB_PADDR, false);
        if (!prxp) {
            nskbl_printf("No recovery blob found, aborting\n");
            return -2;
        }
    }
    if (!prxp->eex_ports->kbl_param) {  // not initialized yet
        nskbl_printf("Recovery blob was not initialized yet, initializing in LIB mode...\n");
        struct eex_param_s tmp_eex_params = {.boot_mode = BOOTSTRAP_MODE_LIB,
                                             .get_hwcfg_patched = (int (*)(patchedHwcfgStruct *))(*(uint32_t *)NSKBL_EXPORTS(NSKBL_EXPORTS_GET_HWCFG_N)),
                                             .stage2_config = NULL};
        ret = prxp->main->init(&tmp_eex_params);
        if (ret < 0) {
            nskbl_printf("Failed to initialize recovery blob: 0x%08X\n", ret);
            return -3;
        }
    }
    DACR_OFF(_r = prxp;);
    return 0;
}
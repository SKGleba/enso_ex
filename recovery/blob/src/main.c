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
#include "bm_ext.h"
#include "nskbl.h"
#include "paper.h"
#include "main.h"
#include "stage2.h"
#include "stage3.h"
#include "utils.h"
#include "bootstrap.h"
#include "view.h"
#include "fmgr.h"

struct eex_param_s g_eex_params = {
    .boot_mode = BOOTSTRAP_MODE_EMMC,
    .get_hwcfg_patched = NULL,
};

ex_ports_struct g_eex_ports = {
    .module_dir = (char*)((uint32_t)E2X_MAGIC) // magic for enso_ex to fill
};

static struct menu_s *current_menu = NULL;

uint8_t session_id[SESSION_UID_KBLP_SIZE];

static void menu_change_selection(struct menu_s *menu, int new_selection) {
    // clear the previous selection
    paper_draw_rectangle(menu->paper, 0,
                         menu->paper->padding.outer.y + (menu->selection * (menu->paper->pen.width.y + menu->paper->padding.inner.y) - menu->paper->padding.inner.y),
                         menu->paper->max.x - menu->paper->min.x, menu->paper->pen.width.y + (2 * menu->paper->padding.inner.y), menu->paper->color,
                         menu->paper->padding.inner.y >> 1);
    menu->selection = new_selection;
	// draw the new selection
	paper_draw_rectangle(menu->paper, 0, menu->paper->padding.outer.y + (menu->selection * (menu->paper->pen.width.y + menu->paper->padding.inner.y)) - menu->paper->padding.inner.y,
						 menu->paper->max.x - menu->paper->min.x, menu->paper->pen.width.y + (2 * menu->paper->padding.inner.y),
						 menu->selector_color, menu->paper->padding.inner.y >> 1);
}

int deinit(void) {
    ILOG("Deinitializing baremetal payload...\n");
    rmemblock_stop();
    if (g_eex_params.boot_mode != BOOTSTRAP_MODE_LIB) {
        log_outputs &= ~LOG_TARGET_LOGPAPER;
        log_outputs &= ~LOG_TARGET_FRONTPAGE;
        bmx_displaymgr(DISPLAYMGR_NSTATE_OFF, DISPLAYMGR_OPT_INCLUDE_FB);  // turn off & free fb
        delay(4000);
    }
    ILOG("Deinitialized baremetal payload!\n");
    return 0;
}

int init(struct eex_param_s *eex_params) {
    unsigned int *bss = &__bss_start__;
    while (bss < &__bss_end__)
        *bss++ = 0;
    rmemblock_start();
    
    log_outputs = LOG_TARGET_CONSOLE;
	ILOG("Baremetal payload started!\n");
    if (eex_params) {
        memcpy(&g_eex_params, eex_params, sizeof(struct eex_param_s));
        DLOG("Using eex_params: boot_mode=%d, get_hwcfg_patched=0x%08X\n", g_eex_params.boot_mode, g_eex_params.get_hwcfg_patched);
        g_eex_params.get_hwcfg_patched((patchedHwcfgStruct *)&g_eex_ports);  // call the patched get_hwcfg function
    } else
        WLOG("WARNING: eex_params is NULL, using default values!\n");

    if (!g_eex_ports.kbl_param) {
        WLOG("WARNING: kbl_param was not given, using nskbl\n");
        g_eex_ports.kbl_param = ns_kbl_param;
    }
    sysroot_init((struct sysroot_buffer *)g_eex_ports.kbl_param);
    DLOG("Setting Session ID:\n");
    memcpy(session_id, g_eex_ports.kbl_param + SESSION_UID_KBLP_OFF, SESSION_UID_KBLP_SIZE);
    hexdump(session_id, SESSION_UID_KBLP_SIZE);

    DLOG("initializing cdram & syscon...\n");
    cdram_enable();
	syscon_init();

    if (g_eex_params.boot_mode != BOOTSTRAP_MODE_LIB) {
        DLOG("initializing display & views...\n");
        if (bmx_displaymgr(DISPLAYMGR_NSTATE_ON, DISPLAYMGR_OPT_INCLUDE_FB) < 0) {
            ELOG("Failed to initialize display!\n");
            return -1;
        }
        if (view_init() < 0) {
            ELOG("Failed to initialize views!\n");
            return -1;
        }
    } else
        DLOG("Skipping display & views initialization [BOOTSTRAP_MODE_LIB]\n");

    if (eex_params->stage2_config)
        memcpy(&stage2_opts, eex_params->stage2_config, sizeof(struct stage2_options));

    ILOG("Init done!\n");
    return 0;
}

static int log_view_handler(enum VIEW_ASSIGNS *next_uview) {
    view_switch(VIEW_DEFAULT);  // switch to the log view

    int buttons = 0;
    while (1) {
        // Wait for user input
        buttons = bmx_ctrl_wait(CTRL_R | CTRL_L | CTRL_PSBUTTON, 4000, 1);
        if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_R)) {
            ILOG("Switching to menu view...\n");
            *next_uview = VIEW_MENU;
            return 0;
        } else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_L)) {
            ILOG("Switching to the file manager view...\n");
            *next_uview = VIEW_FMGR;
            return 0;
        } else if (BMX_CTRL_BUTTON_HELD(buttons, CTRL_PSBUTTON)) {
            DLOG("PS button pressed - temp display state toggle\n");
            bmx_displaymgr(DISPLAYMGR_NSTATE_TOGGLE, DISPLAYMGR_OPT_DISP_ONLY);
        }
    }
    return 0;
}

static int menu_view_handler(enum VIEW_ASSIGNS *next_uview) {
    view_switch(VIEW_MENU);  // switch to the menu view

    int ret = 0;
    current_menu->prs_buttons = 0;
    while (current_menu) {
        current_menu->prs_buttons = bmx_ctrl_wait(CTRL_UP | CTRL_DOWN | CTRL_L | CTRL_R | current_menu->exp_buttons | CTRL_PSBUTTON, 4000, 1);

        if (BMX_CTRL_BUTTON_HELD(current_menu->prs_buttons, CTRL_DOWN) && (current_menu->selection < current_menu->entry_count - 1))
            menu_change_selection(current_menu, current_menu->selection + 1);
        else if (BMX_CTRL_BUTTON_HELD(current_menu->prs_buttons, CTRL_UP) && (current_menu->selection > 0))
            menu_change_selection(current_menu, current_menu->selection - 1);
        else if (BMX_CTRL_BUTTON_HELD(current_menu->prs_buttons, CTRL_L)) {
            ILOG("Switching to log view...\n");
            *next_uview = VIEW_DEFAULT;
            return 0;
        } else if (BMX_CTRL_BUTTON_HELD(current_menu->prs_buttons, CTRL_R)) {
            ILOG("Switching to the file manager view...\n");
            *next_uview = VIEW_FMGR;
            return 0;
        } else if (~current_menu->prs_buttons & current_menu->exp_buttons) {
            ret = current_menu->select(current_menu->selection);
            switch (ret) {
                case MENU_RET_CONTINUE:
                    break;  // continue the loop
                case MENU_RET_FINISH:
                    current_menu = NULL;  // exit the menu loop
                    *next_uview = VIEW_COUNT;  // exit view_handler looper
                    break;
                case MENU_RET_FINISH_DEINIT:
                    current_menu = NULL;  // exit the menu loop and deinit
                    *next_uview = VIEW_COUNT;
                    deinit();
                    break;
                default:
                    ELOG("ERROR: Unknown menu return value %d\n", ret);
                    break;
            }
        } else if (BMX_CTRL_BUTTON_HELD(current_menu->prs_buttons, CTRL_PSBUTTON)) {
            DLOG("PS button pressed - temp display state toggle\n");
            bmx_displaymgr(DISPLAYMGR_NSTATE_TOGGLE, DISPLAYMGR_OPT_DISP_ONLY);
        }
    }
    return ret;
}

int main(int stage) {
    // TITLES
    ILOG("Initializing the default views..\n");
    if (stage == 2 || stage == 0xB) { // keeps the logs & titles going from stage 2 to 3
        paper_area(&default_paper, DFL_PAPER_START_X, DFL_PAPER_START_Y, DFL_PAPER_END_X, DFL_PAPER_END_Y);
        for (int i = 0; i < VIEW_TEMP; i++) {
            default_paper.view_idx = i;                  // set the view index for each paper
            paper_clear(&default_paper, BG_PAPER_COLR);  // reset the whole screen
            pen_reset(&default_paper, TITLE_PEN_COLR);
            switch (i) {
                case VIEW_DEFAULT:
                    pprintf_align(&default_paper, LEFT, "<<(L) FILES");
                    pprintf_align(&default_paper, RIGHT, "MENU (R)>>");
                    break;
                case VIEW_MENU:
                    pprintf_align(&default_paper, LEFT, "<<(L) LOGS");
                    pprintf_align(&default_paper, RIGHT, "FILES (R)>>");
                    break;
                case VIEW_FMGR:
                    pprintf_align(&default_paper, LEFT, "<<(L) MENU");
                    pprintf_align(&default_paper, RIGHT, "LOGS (R)>>");
                    break;
            }
            pen_reset(&default_paper, TITLE_PEN_COLR);
            pprintf_align(&default_paper, CENTER, "enso_ex recovery menu\nby skgleba\n");
        }

        // LOG
        default_paper.view_idx = VIEW_DEFAULT;  // set the view index for the log paper
        paper_area(&default_paper, LOG_PAPER_START_X, LOG_PAPER_START_Y, LOG_PAPER_END_X, LOG_PAPER_END_Y);
        paper_clear(&default_paper, LOG_PAPER_COLR);
        pen_reset(&default_paper, LOG_PEN_COLR);
        default_paper.blank_mode = LOG_PAPER_BLANK_MODE;
    }
    log_outputs |= LOG_TARGET_LOGPAPER;

    // FMGR
    DLOG("Initializing the file manager view..\n");
    fmgr_init();  // initialize the file manager

    // MENU
    DLOG("Initializing the menu view..\n");
    // -- status
    if (stage != 3) { // keep from s2
        paper_clear(&status_paper, MENU_PAPER_COLR);
        pen_reset(&status_paper, STATUS_PEN_COLR);
        pprintf_align(&status_paper, LEFT, "S%X:\n", stage);
    }
    // -- info
    paper_clear(&info_paper, INFO_PAPER_COLR);
    pen_reset(&info_paper, INFO_PEN_COLR);
    log_outputs |= LOG_TARGET_FRONTPAGE;
    ULOG("Welcome to the enso_ex recovery menu!\n");
    ULOG("Use arrow keys to navigate, X to select.\n");
    ULOG("Pressing L/R will switch active views.\n");
    if (stage == 2) {
        ULOG("START will apply changes and continue boot.\n");
        if (g_eex_params.boot_mode == BOOTSTRAP_MODE_EMMC)
            ULOG("SELECT will make the changes persistent.\n");
    }
    // -- options
    paper_clear(&menu_paper, MENU_PAPER_COLR);
    pen_reset(&menu_paper, MENU_PEN_COLR);
    switch (stage) {
        case 2:
            current_menu = &stage2_menu_s;
            current_menu->draw(-1);                  // initial draw
            menu_change_selection(current_menu, 0);  // set initial selection
            break;
		case 3:
        case 0xB: // bootmgr is at the same level as stage 3
			current_menu = &stage3_menu_s;
			current_menu->draw(-1);
			menu_change_selection(current_menu, 0);
			break;
        default:
            DLOG("ERROR: Unknown stage %d\n", stage);
            break;
    }

    ILOG("Menu view initialized, switching to it\n");
    enum VIEW_ASSIGNS user_view = VIEW_MENU;
    int ret = 0;
    while (user_view < VIEW_COUNT) {
        switch(user_view) {
            case VIEW_DEFAULT:
                ret = log_view_handler(&user_view);
                break;
            case VIEW_MENU:
                ret = menu_view_handler(&user_view);
                break;
            case VIEW_FMGR:
                ret = fmgr_view_handler(&user_view);
                break;
        }
    }

    log_outputs &= ~LOG_TARGET_FRONTPAGE;
    log_outputs &= ~LOG_TARGET_LOGPAPER;

    ILOG("Exiting baremetal payload...\n");
    if (ret >= 0)
        ret = 0;

    return ret;
}

__attribute__((section(".text.start"))) int start(struct eex_param_s *eex_params) {
    if ((uint32_t)eex_params == E2X_MAGIC) { // special stage3 mode
        struct eex_param_s tmp_eex_params = {
            .boot_mode = BOOTSTRAP_MODE_BOOTMGR,
            .get_hwcfg_patched = (int (*)(patchedHwcfgStruct *))(*(uint32_t*)NSKBL_EXPORTS(NSKBL_EXPORTS_GET_HWCFG_N)),
            .stage2_config = NULL
        };
        init(&tmp_eex_params);
        return main(0xB);
    } else
        init(eex_params);
    return main(2);
}

// ---- PAPERS ----
struct paper_s menu_paper = {
    .view_idx = VIEW_MENU,
	.min = {MENU_PAPER_START_X, MENU_PAPER_START_Y},
	.max = {MENU_PAPER_END_X, MENU_PAPER_END_Y},
	.color = MENU_PAPER_COLR,
	.pen = {
        .pos = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y},
        .width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
        .color = MENU_PEN_COLR
    },
	.blank_mode = DFL_PAPER_BLANK_MODE,
	.align = PAPER_ALIGN_LEFT,
	.padding = {
		.inner = {DFL_PAPER_IPAD_X, MENU_PAPER_INNER_PADDING_Y},
		.outer = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
	}
};

struct paper_s status_paper = {
    .view_idx = VIEW_MENU,
    .min = {STATUS_PAPER_START_X, STATUS_PAPER_START_Y},
    .max = {STATUS_PAPER_END_X, STATUS_PAPER_END_Y},
    .color = STATUS_PAPER_COLR,
    .pen = {
        .pos = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y},
        .width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
        .color = STATUS_PEN_COLR
    },
	.blank_mode = DFL_PAPER_BLANK_MODE,
	.align = PAPER_ALIGN_LEFT,
	.padding = {
		.inner = {DFL_PAPER_IPAD_X, DFL_PAPER_IPAD_Y},
		.outer = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
	}
};

struct paper_s info_paper = {
    .view_idx = VIEW_MENU,
    .min = {INFO_PAPER_START_X, INFO_PAPER_START_Y},
    .max = {INFO_PAPER_END_X, INFO_PAPER_END_Y},
    .color = INFO_PAPER_COLR,
    .pen = {
        .pos = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y},
        .width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
        .color = INFO_PEN_COLR
    },
    .blank_mode = INFO_PAPER_BLANK_MODE,
    .align = PAPER_ALIGN_LEFT,
    .padding = {
        .inner = {DFL_PAPER_IPAD_X, DFL_PAPER_IPAD_Y},
        .outer = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
    }
};
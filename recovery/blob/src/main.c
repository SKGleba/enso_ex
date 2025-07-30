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


struct pen_dets menu_pen = {
	.pos = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
	.width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
	.color = MENU_PEN_COLR
};
struct paper_dets menu_paper = {
    .view_idx = 0,
	.min = {MENU_PAPER_START_X, MENU_PAPER_START_Y},
	.max = {MENU_PAPER_END_X, MENU_PAPER_END_Y},
	.color = MENU_PAPER_COLR,
	.pen = &menu_pen,
	.blank_mode = DFL_PAPER_BLANK_MODE,
	.align = PAPER_ALIGN_LEFT,
	.padding = {
		.inner = {DFL_PAPER_IPAD_X, MENU_PAPER_INNER_PADDING_Y},
		.outer = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
	}
};

struct pen_dets status_pen = {
	.pos = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
	.width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
	.color = STATUS_PEN_COLR
};
struct paper_dets status_paper = {
    .view_idx = 0,
    .min = {STATUS_PAPER_START_X, STATUS_PAPER_START_Y},
    .max = {STATUS_PAPER_END_X, STATUS_PAPER_END_Y},
    .color = STATUS_PAPER_COLR,
    .pen = &status_pen,
	.blank_mode = DFL_PAPER_BLANK_MODE,
	.align = PAPER_ALIGN_LEFT,
	.padding = {
		.inner = {DFL_PAPER_IPAD_X, DFL_PAPER_IPAD_Y},
		.outer = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
	}
};

struct eex_param_s g_eex_params = {
    .init_os0 = NULL,
    .load_exe = NULL,
    .get_hwcfg_patched = NULL,
    .kbl_param = NULL,
    .disable_bootarea_update = NULL
};

static struct menu_s *current_menu = NULL;

static void menu_change_selection(struct menu_s *menu, int new_selection) {
    // clear the previous selection
    paper_draw_rectangle(menu->paper, 0,
                         menu->paper->padding.outer.y + (menu->selection * (menu->paper->pen->width.y + menu->paper->padding.inner.y) - menu->paper->padding.inner.y),
                         menu->paper->max.x - menu->paper->min.x, menu->paper->pen->width.y + (2 * menu->paper->padding.inner.y), menu->paper->color,
                         menu->paper->padding.inner.y >> 1);
    menu->selection = new_selection;
	// draw the new selection
	paper_draw_rectangle(menu->paper, 0, menu->paper->padding.outer.y + (menu->selection * (menu->paper->pen->width.y + menu->paper->padding.inner.y)) - menu->paper->padding.inner.y,
						 menu->paper->max.x - menu->paper->min.x, menu->paper->pen->width.y + (2 * menu->paper->padding.inner.y),
						 menu->selector_color, menu->paper->padding.inner.y >> 1);
}

static int deinit(struct sysroot_buffer *sysroot) {
    LOG("Deinitializing baremetal payload...\n");

    g_log_targets &= ~LOG_TARGET_PAPER;  // disable paper logging

    bmx_display_deinit();
    delay(4000);

    LOG("Deinitialized baremetal payload!\n");

    return 0;
}

static int init(struct eex_param_s *eex_params) {
    g_log_targets = LOG_TARGET_CONSOLE;
	LOG("Baremetal payload started!\n");

    if (eex_params)
        memcpy(&g_eex_params, eex_params, sizeof(struct eex_param_s));
    else
        LOG("WARNING: eex_params is NULL, using default values!\n");

    if (!g_eex_params.kbl_param) {
        LOG("WARNING: kbl_param was not given, using nskbl\n");
        g_eex_params.kbl_param = ns_kbl_param;
    }
    sysroot_init((struct sysroot_buffer *)g_eex_params.kbl_param);

    LOG("initializing cdram & syscon...\n");
    cdram_enable();
	syscon_init();

    LOG("initializing display & views...\n");
    int ret = 0;
	if (sysroot_model_is_dolce())
        ret = bmx_display_init(DISPLAY_TYPE_HDMI, VIEW_COUNT);
    else if (sysroot_model_is_vita2k())
        ret = bmx_display_init(DISPLAY_TYPE_LCD, VIEW_COUNT);
    else
        ret = bmx_display_init(DISPLAY_TYPE_OLED, VIEW_COUNT);
    if (ret < 0) {
        LOG("Failed to initialize display!\n");
        return ret;
    }

    if (view_init() < 0) {
        LOG("Failed to initialize views!\n");
        return -1;
    }

    LOG("Init done!\n");
    return 0;
}

int main(int stage) {
    paper_clear(&default_paper, BG_PAPER_COLR);  // reset the whole screen

    // TITLE
    pen_reset(&default_pen, TITLE_PEN_COLR);
    pprintf_align(&default_paper, CENTER, "enso_ex recovery menu\nby skgleba\n");

    // STATUS
    paper_clear(&status_paper, MENU_PAPER_COLR);
    pen_reset(&status_pen, STATUS_PEN_COLR);
    pprintf_align(&status_paper, LEFT, "S%d\n", stage);

    // LOG
    paper_area(&default_paper, LOG_PAPER_START_X, LOG_PAPER_START_Y, LOG_PAPER_END_X, LOG_PAPER_END_Y);
    paper_clear(&default_paper, LOG_PAPER_COLR);
    pen_reset(&default_pen, LOG_PEN_COLR);
    default_paper.blank_mode = LOG_PAPER_BLANK_MODE;
    g_log_targets |= LOG_TARGET_PAPER;
    scrlog("Welcome to the enso_ex recovery menu!\n");
    scrlog("Use arrow keys to navigate, X to select.\n");

    // MENU
    paper_clear(&menu_paper, MENU_PAPER_COLR);
    pen_reset(&menu_pen, MENU_PEN_COLR);
    switch (stage) {
        case 2:
            current_menu = &stage2_menu_s;
            current_menu->draw(-1);                  // initial draw
            menu_change_selection(current_menu, 0);  // set initial selection
            break;
		case 3:
			current_menu = &stage3_menu_s;
			current_menu->draw(-1);                  // initial draw
			menu_change_selection(current_menu, 0);  // set initial selection
			break;
        default:
            LOG("ERROR: Unknown stage %d\n", stage);
            break;
    }

    int ret = 0;
    current_menu->prs_buttons = 0;
    while (current_menu) {
        current_menu->prs_buttons = bmx_ctrl_wait(CTRL_UP | CTRL_DOWN | current_menu->exp_buttons, 4000, 1);

        if (BMX_CTRL_BUTTON_HELD(current_menu->prs_buttons, CTRL_DOWN) && (current_menu->selection < current_menu->entry_count - 1))
            menu_change_selection(current_menu, current_menu->selection + 1);
        else if (BMX_CTRL_BUTTON_HELD(current_menu->prs_buttons, CTRL_UP) && (current_menu->selection > 0))
            menu_change_selection(current_menu, current_menu->selection - 1);
        else if (~current_menu->prs_buttons & current_menu->exp_buttons) {
            ret = current_menu->select(current_menu->selection);
            switch (ret) {
                case MENU_RET_CONTINUE:
                    break;  // continue the loop
                case MENU_RET_FINISH:
                    current_menu = NULL;  // exit the menu loop 
                    break;
                case MENU_RET_FINISH_DEINIT:
                    current_menu = NULL;  // exit the menu loop and deinit
                    deinit((struct sysroot_buffer *)g_eex_params.kbl_param);
                    break;
                default:
                    LOG("ERROR: Unknown menu return value %d\n", ret);
                    break;
            }
        }
    }

    g_log_targets &= ~LOG_TARGET_PAPER;  // disable paper logging 

    LOG("Exiting baremetal payload...\n");

    return ret;
}

__attribute__((section(".text.start"), optimize("O0"))) int start(struct eex_param_s *eex_params) {
    init(eex_params);
    return main(2);
}
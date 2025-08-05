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
#include "fmgr.h"
#include "main.h"
#include "nskbl.h"
#include "paper.h"
#include "stageB.h"
#include "stor.h"
#include "utils.h"
#include "view.h"

struct menu_s stageB_menu_s = {.draw = stageB_menu,
                               .select = stageB_menu,
                               .entry_count = 1,
                               .selection = 0,
                               .exp_buttons = CTRL_CROSS | CTRL_START,
                               .prs_buttons = 0,
                               .paper = &menu_paper,
                               .selector_color = MENU_SELECTOR_COLR};

int stageB_menu(int selection) {
    if (selection < 0) {  // initial draw
        paper_clear(stageB_menu_s.paper, stageB_menu_s.paper->color);
        pen_reset(stageB_menu_s.paper, stageB_menu_s.paper->pen.color);
        pprintf(stageB_menu_s.paper, "1. Block writes to boot sectors\n");
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stageB_menu_s.prs_buttons, CTRL_CROSS)) {
        switch (selection) {
            case 0:
                if (g_disable_bootarea_update) {
                    g_disable_bootarea_update = 0;
                    DACR_OFF(*(g_eex_ports.protect_boot) = 0);
					LOG("Boot area protection disabled\n");
				} else {
					g_disable_bootarea_update = 1;
					DACR_OFF(*(g_eex_ports.protect_boot) = 1);
					LOG("Boot area protection enabled\n");
                }
				return MENU_RET_CONTINUE;
            default:
                LOG("Invalid selection %d\n", selection);
                return MENU_RET_CONTINUE;  // continue the loop
        }
    } else if (BMX_CTRL_BUTTON_HELD(stageB_menu_s.prs_buttons, CTRL_START))
        return MENU_RET_FINISH_DEINIT;
    return MENU_RET_CONTINUE;
}
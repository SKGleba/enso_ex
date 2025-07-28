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

struct menu_s stage3_menu_s = {.draw = stage3_menu,
                               .select = stage3_menu,
                               .entry_count = 2,
                               .selection = 0,
                               .exp_buttons = CTRL_CROSS,
                               .prs_buttons = 0,
                               .paper = &menu_paper,
                               .selector_color = MENU_SELECTOR_COLR};

int stage3_menu(int selection) {
    if (selection < 0) {  // initial draw
        ppaper_clear(stage3_menu_s.paper, stage3_menu_s.paper->color);
        ppen_reset(stage3_menu_s.paper->pen, stage3_menu_s.paper->pen->color);
        pprintf(stage3_menu_s.paper, "1. Continue boot\n");
        pprintf(stage3_menu_s.paper, "2. Some option\n");
        return 0;
    }
    if (BMX_CTRL_BUTTON_HELD(stage3_menu_s.prs_buttons, CTRL_CROSS)) {
        switch (selection) {
            case 0:  // Continue boot
                return MENU_RET_FINISH_DEINIT;  // deinit & exit
            default:
                scrprintf("Invalid selection %d\n", selection);
                return MENU_RET_CONTINUE;  // continue the loop
        }
    }
    return MENU_RET_CONTINUE;  // continue the loop
}

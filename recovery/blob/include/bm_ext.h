#ifndef __BM_EXT_H__
#define __BM_EXT_H__

#include <baremetal/display.h>
#include <baremetal/ctrl.h>

enum DISPLAYMGR_NSTATE {
	DISPLAYMGR_NSTATE_OFF = 0,
	DISPLAYMGR_NSTATE_ON,
	DISPLAYMGR_NSTATE_TOGGLE
};

enum DISPLAYMGR_OPT {
	DISPLAYMGR_OPT_INCLUDE_FB = 0,
	DISPLAYMGR_OPT_DISP_ONLY,
};

// workaround for libbaremetal's broken ctrl_read function
#define BMX_CTRL_BUTTON_HELD(ctrl, button) !((ctrl) & (button))
#define BMX_CTRL_BUTTON_PRESSED(ctrl, old, button) !(((ctrl) & ~(old)) & (button))
void bmx_ctrl_read(uint32_t *buttons);
uint32_t bmx_ctrl_wait(uint32_t exp_buttons, uint32_t poll_rate, uint32_t depress);

uint32_t bmx_get_time(int *since_reset);

int bmx_displaymgr(enum DISPLAYMGR_NSTATE enable, enum DISPLAYMGR_OPT opt);

#endif
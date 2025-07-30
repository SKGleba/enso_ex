#ifndef __BM_EXT_H__
#define __BM_EXT_H__

#include <baremetal/display.h>
#include <baremetal/ctrl.h>

// workaround for libbaremetal's broken ctrl_read function
#define BMX_CTRL_BUTTON_HELD(ctrl, button) !((ctrl) & (button))
#define BMX_CTRL_BUTTON_PRESSED(ctrl, old, button) !(((ctrl) & ~(old)) & (button))
void bmx_ctrl_read(uint32_t *buttons);
uint32_t bmx_ctrl_wait(uint32_t exp_buttons, uint32_t poll_rate, uint32_t depress);

void bmx_display_deinit(void);
int bmx_display_init(enum display_type type, int view_count);
uint32_t bmx_get_time(int *since_reset);

#endif
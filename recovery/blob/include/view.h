#ifndef VIEW_H
#define VIEW_H

#include <baremetal/display.h>

#define VIEW_COUNT 4
#define VIEW_DEFAULT 0

struct frame_s {
    uint32_t *addr;
    uint32_t size;
    int width; // on PSP2, this == pitch
    int height;
	int pixcount;
};

extern int view_current;
extern uint32_t *view_vas[VIEW_COUNT];
extern struct frame_s view_frame;

int view_init(void);
void view_switch(int new_view);

#define view_draw_pixel(_idx, _x, _y, _colr) {view_vas[_idx][((_y) * view_frame.width) + (_x)] = _colr;}

#endif // VIEW_H
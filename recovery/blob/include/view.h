#ifndef VIEW_H
#define VIEW_H

#include <baremetal/display.h>

#define VIEW_COUNT 4

enum VIEW_ASSIGNS {
    VIEW_DEFAULT = 0, // default (log) view
    VIEW_MENU, // main menu view
    VIEW_FMGR, // file manager view
    VIEW_TEMP // temporary view for various purposes
};

struct frame_s {
    uint32_t *addr;
    uint32_t size;
    int width; // on PSP2, this == pitch
    int height;
	int pixcount;
};

#define view_draw_pixel(_idx, _x, _y, _colr) {view_vas[_idx][((_y) * view_frame.width) + (_x)] = _colr;}

#ifndef RXP_PIE
extern int view_current;
extern uint32_t *view_vas[VIEW_COUNT];
extern struct frame_s view_frame;

int view_init(void);
void view_switch(enum VIEW_ASSIGNS new_view);
int view_copy(enum VIEW_ASSIGNS src_view, enum VIEW_ASSIGNS dst_view);
#else
#define r_view_current *(_r->view->view_current)
#define r_view_vas (_r->view->view_vas)
#define r_view_frame (_r->view->view_frame)
#define r_view_init(...) _r->view->view_init(__VA_ARGS__)
#define r_view_switch(...) _r->view->view_switch(__VA_ARGS__)
#define r_view_copy(...) _r->view->view_copy(__VA_ARGS__)
#endif

struct exports_view_s {
    int *view_current;
    uint32_t **view_vas;
    struct frame_s *view_frame;
    int (*view_init)(void);
    void (*view_switch)(enum VIEW_ASSIGNS new_view);
    int (*view_copy)(enum VIEW_ASSIGNS src_view, enum VIEW_ASSIGNS dst_view);
};

#endif // VIEW_H
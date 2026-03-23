#include <baremetal/display.h>
#include "view.h"
#include "utils.h"

uint32_t *view_vas[VIEW_COUNT];
struct frame_s view_frame = {
	.addr = NULL,
	.size = 0,
	.width = 0,
	.height = 0,
	.pixcount = 0
};

int view_current = VIEW_DEFAULT;

int view_init(void) {
    const struct display_config *config = display_get_current_config();
	if (!config) {
		ELOG("No display config available!\n");
		return -1;
	}
    view_frame.addr = (uint32_t *)config->addr;
    if (!view_frame.addr) {
		ELOG("No framebuffer address available!\n");
		return -1;
	}
	view_frame.width = config->width;
	view_frame.height = config->height;
	view_frame.pixcount = view_frame.width * view_frame.height;
    view_frame.size = view_frame.pixcount * 4;
    if (!view_frame.size) {
		ELOG("Invalid framebuffer size: %d\n", view_frame.size);
		return -1;
	}
    for (int i = 0; i < VIEW_COUNT; i++) {
        view_vas[i] = (uint32_t *)(view_frame.addr + ((i + 1) * view_frame.pixcount));
		memset(view_vas[i], 0, view_frame.size);
    }

	view_current = VIEW_DEFAULT;
	view_vas[view_current] = view_frame.addr;

	DLOG("View initialized: fva=%08X, fsize=%08X, width=%d, height=%d, pixcount=%d\n",
		view_frame.addr, view_frame.size, view_frame.width, view_frame.height, view_frame.pixcount);
	return 0;
}

void view_switch(enum VIEW_ASSIGNS new_view) {
    view_vas[view_current] = (uint32_t *)(view_frame.addr + ((view_current + 1) * view_frame.pixcount));
    memcpy(view_vas[view_current], view_frame.addr, view_frame.size);

	view_current = new_view;

    memcpy(view_frame.addr, view_vas[view_current], view_frame.size);
    view_vas[view_current] = view_frame.addr;
}

int view_copy(enum VIEW_ASSIGNS src_view, enum VIEW_ASSIGNS dst_view) {
    if (src_view < 0 || src_view >= VIEW_COUNT || dst_view < 0 || dst_view >= VIEW_COUNT) {
		ELOG("Invalid view index: src=%d, dst=%d\n", src_view, dst_view);
		return -1;
	}
	memcpy(view_vas[dst_view], view_vas[src_view], view_frame.size);
	DLOG("Copied view %d to view %d\n", src_view, dst_view);
	return 0;
}
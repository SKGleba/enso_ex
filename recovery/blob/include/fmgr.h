#ifndef __FMGR_H__
#define __FMGR_H__

#include <baremetal/draw.h>

#include "utils.h"

enum FMGR_PAPERS {
	FMGR_PAPERS_LV = 0, // left panel
	FMGR_PAPERS_RV, // right panel
	FMGR_PAPERS_IB, // info panel
	FMGR_PAPERS_LB, // left border
	FMGR_PAPERS_RB, // right border
	FMGR_PAPERS_END
};

enum FMGR_MAIN_VIEWS_PARAMS {
    FMGR_LRV_PAPER_COLR = RGBA8(0x20, 0x20, 0x20, 0xFF),
    FMGR_LRV_PEN_COLR = WHITE,
    FMGR_LRV_SELECTOR_COLR = GREEN,
    FMGR_LRV_PAPER_START_Y = 60,
    FMGR_LRV_PAPER_END_Y = 500,
    FMGR_LRV_PAPER_INNER_PADDING_Y = 6,  // px between menu items (for the selector)
    FMGR_LV_PAPER_START_X = 20,
    FMGR_LV_PAPER_END_X = 470,
    FMGR_RV_PAPER_START_X = 490,
    FMGR_RV_PAPER_END_X = 940,
	FMGR_LRV_PAPER_OPAD_X = 8,
};

enum FMGR_BORDER_VIEWS_PARAMS {
    FMGR_ILRB_PAPER_COLR = BG_PAPER_COLR,
    FMGR_LRB_PEN_COLR = GREEN,
    FMGR_IB_PEN_COLR = YELLOW,
    FMGR_ILB_PAPER_START_X = 0,
    FMGR_LB_PAPER_END_X = FMGR_LV_PAPER_START_X,
    FMGR_IB_PAPER_END_X = 960,
    FMGR_RB_PAPER_START_X = FMGR_RV_PAPER_END_X,
    FMGR_RB_PAPER_END_X = 960,
    FMGR_LRB_PAPER_START_Y = FMGR_LRV_PAPER_START_Y,
    FMGR_LRB_PAPER_END_Y = FMGR_LRV_PAPER_END_Y,
    FMGR_IB_PAPER_START_Y = FMGR_LRV_PAPER_END_Y,
    FMGR_IB_PAPER_END_Y = 544,
	FMGR_LRB_PAPER_OPAD_X = 0,
};

#define FMGR_XV_MAX_NAME_LEN 32 // includes the null terminator
#define FMGR_XV_ENTRY_COUNT 16
#define FMGR_MAX_PATH_LEN 256

#define HAS_ENDSLASH(path) ((path)[strlen(path) - 1] == '/')

int fmgr_init(void);
int fmgr_view_handler(int *next_uview);
void fmgr_init_mounts(void);

#endif
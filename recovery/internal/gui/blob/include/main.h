#ifndef __MAIN_H__
#define __MAIN_H__

#include <baremetal/draw.h>

#include "paper.h"

enum MENU_RETURNS {
    MENU_RET_CONTINUE = 0, // continue looping
    MENU_RET_FINISH = 1, // exit the menu loop
    MENU_RET_FINISH_DEINIT = 2, // exit the menu loop and deinit
};

enum MENU_DRAW_PARAMS {
    MENU_PAPER_COLR = RGBA8(0x40, 0x40, 0x40, 0xFF),
    MENU_PAPER_START_X = 20,
    MENU_PAPER_START_Y = 60,
    MENU_PAPER_END_X = 620,
    MENU_PAPER_END_Y = 224,
    MENU_PAPER_INNER_PADDING_Y = 6, // px between menu items (for the selector)
    MENU_PEN_COLR = YELLOW,
    MENU_SELECTOR_COLR = GREEN,
};

enum STATUS_DRAW_PARAMS {
    STATUS_PAPER_COLR = RGBA8(0x40, 0x40, 0x40, 0xFF),
    STATUS_PAPER_START_X = 640,
    STATUS_PAPER_START_Y = 60,
    STATUS_PAPER_END_X = 940,
    STATUS_PAPER_END_Y = 224,
    STATUS_PEN_COLR = CYAN,
};

enum LOG_DRAW_PARAMS {  // uses default_paper and default_pen
    LOG_PAPER_COLR = WHITE,
    LOG_PAPER_START_X = 20,
    LOG_PAPER_START_Y = 244,
    LOG_PAPER_END_X = 940,
    LOG_PAPER_END_Y = 524,
    LOG_PAPER_BLANK_MODE = PAPER_BLANK_MODE_LINE_CLEAR | PAPER_BLANK_MODE_AREA_CLEAR,
    LOG_PEN_COLR = BLACK,
};

#define TITLE_PEN_COLR RGBA8(0xFF, 0xFF, 0xFF, 0xFF)  // uses default_paper and default_pen. draw once
#define BG_PAPER_COLR RGBA8(0xA0, 0x20, 0x80, 0xFF) // radish lul

struct menu_s {
    int (*draw)(int minus_one);
    int (*select)(int selection);
    int entry_count;
    int selection;
    uint32_t exp_buttons;
    uint32_t prs_buttons;
    struct paper_dets *paper;
    uint32_t selector_color;
};

extern struct pen_dets menu_pen;
extern struct paper_dets menu_paper;

extern struct pen_dets status_pen;
extern struct paper_dets status_paper;

extern void (*eex_init_os0)(int mbr_off);

int main(int stage);

#endif /* __MAIN_H__ */
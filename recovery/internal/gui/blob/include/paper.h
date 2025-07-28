#ifndef __PAPER_H__
#define __PAPER_H__

#include <baremetal/display.h>
#include <baremetal/draw.h>
#include <baremetal/font.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nskbl.h"

struct pen_dets {
    struct {
        int x;
        int y;
    } pos;  // Current position of the pen
    struct {
        int x;
        int y;
    } width;         // Width of the pen
    uint32_t color;  // Color of the pen
};
#define DFL_PEN_WIDTH_X 16
#define DFL_PEN_WIDTH_Y 16
#define DFL_PEN_COLR WHITE

struct paper_dets {
    struct {
        int x;
        int y;
    } min, max;
    uint32_t color;
    struct pen_dets *pen;
    int blank_mode;
    int align;
    struct {
        struct {
            int x;
            int y;
        } inner;
        struct {
            int x;
            int y;
        } outer;
    } padding;
};

enum PAPER_ALIGN {
    PAPER_ALIGN_LEFT,
    PAPER_ALIGN_CENTER,
    PAPER_ALIGN_RIGHT
};

enum PAPER_BLANK_MODE {
    PAPER_BLANK_MODE_NONE,  // Normal mode
    PAPER_BLANK_MODE_LINE_CLEAR = 0b1,  // Clear the line when entering a new line
    PAPER_BLANK_MODE_AREA_CLEAR = 0b10,  // Clear the area when wrapping around the paper
};

#define DFL_PAPER_START_X 0
#define DFL_PAPER_START_Y 0
#define DFL_PAPER_END_X 960
#define DFL_PAPER_END_Y 544
#define DFL_PAPER_COLR BLACK
#define DFL_PAPER_IPAD_X 0
#define DFL_PAPER_IPAD_Y 2
#define DFL_PAPER_OPAD_X DFL_PEN_WIDTH_X
#define DFL_PAPER_OPAD_Y DFL_PEN_WIDTH_Y
#define DFL_PAPER_BLANK_MODE PAPER_BLANK_MODE_NONE

extern struct pen_dets default_pen;
extern struct paper_dets default_paper;

#define paper_clear(paper, colr)                                                         \
    do {                                                                                   \
        paper.color = colr;                                                               \
        draw_rectangle(paper.min.x, paper.min.y, paper.max.x - paper.min.x, paper.max.y - paper.min.y, colr);   \
    } while (0)
#define ppaper_clear(paper, colr) \
    do {                                       \
        paper->color = colr;                           \
        draw_rectangle(paper->min.x, paper->min.y, paper->max.x - paper->min.x, paper->max.y - paper->min.y, colr); \
    } while (0)

#define paper_area(paper, min_x, min_y, max_x, max_y) \
    do {                                       \
        paper.min.x = min_x;                   \
        paper.min.y = min_y;                   \
        paper.max.x = max_x;                   \
        paper.max.y = max_y;                   \
    } while (0)
#define ppaper_area(paper, min_x, min_y, max_x, max_y) \
    do {                                       \
        paper->min.x = min_x;                   \
        paper->min.y = min_y;                   \
        paper->max.x = max_x;                   \
        paper->max.y = max_y;                   \
    } while (0)

#define pen_reset(pen, colr)                        \
    do {                                       \
        pen.width.x = DFL_PEN_WIDTH_X;       \
        pen.width.y = DFL_PEN_WIDTH_Y;       \
        pen.pos.x = pen.width.x;            \
        pen.pos.y = pen.width.y;            \
        pen.color = colr;                    \
    } while (0)
#define ppen_reset(pen, colr)                        \
    do {                                       \
        pen->width.x = DFL_PEN_WIDTH_X;       \
        pen->width.y = DFL_PEN_WIDTH_Y;       \
        pen->pos.x = pen->width.x;            \
        pen->pos.y = pen->width.y;            \
        pen->color = colr;                    \
    } while (0)
#define pen_pos(pen, x, y)  \
    do {               \
        pen.pos.x = x; \
        pen.pos.y = y; \
    } while (0)
#define ppen_pos(pen, x, y)  \
    do {               \
        pen->pos.x = x; \
        pen->pos.y = y; \
    } while (0)

void paper_write(struct paper_dets *paper, const char *text, int count);
void paper_print(struct paper_dets *paper, const char *text, int align, int count);
void paper_printf(struct paper_dets *paper, const char *fmt, ...);

#define pprintf(_paper, fmt, ...) \
    paper_printf((_paper), fmt, ##__VA_ARGS__);

#define pprintf_align(_paper, _align, fmt, ...) \
    do { \
        int _prev_align = (_paper)->align; \
        (_paper)->align = PAPER_ALIGN_##_align; \
        paper_printf(_paper, fmt, ##__VA_ARGS__); \
        (_paper)->align = _prev_align; \
    } while (0)

#define scrprintf(fmt, ...) \
    pprintf((&default_paper), fmt, ##__VA_ARGS__);

#define scrprintf_align(_align, fmt, ...) \
    pprintf_align((&default_paper), _align, fmt, ##__VA_ARGS__);

void paper_draw_rectangle(struct paper_dets *paper, int x, int y, int width, int height, uint32_t color, int fill_pixels);

#endif /* __PAPER_H__ */
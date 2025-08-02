#ifndef __PAPER_H__
#define __PAPER_H__

#include <baremetal/display.h>
#include <baremetal/draw.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nskbl.h"
#include "view.h"
#include "utils.h"

#define DFL_PEN_WIDTH_X 16
#define DFL_PEN_WIDTH_Y 16
#define DFL_PEN_COLR WHITE

struct paper_s {
    int view_idx;
    struct {
        int x;
        int y;
    } min, max;
    uint32_t color;
    struct {
        struct {
            int x;
            int y;
        } pos;  // Current position of the pen
        struct {
            int x;
            int y;
        } width;         // Width of the pen
        uint32_t color;  // Color of the pen
    } pen;
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

extern struct paper_s default_paper;

#define paper_clear(_paper, _colr)                                                                                               \
    do {                                                                                                                         \
        (_paper)->color = _colr;                                                                                                   \
        paper_draw_rectangle((_paper), 0, 0, (_paper)->max.x - (_paper)->min.x, (_paper)->max.y - (_paper)->min.y, _colr, 0); \
    } while (0)

#define paper_area(_paper, min_x, min_y, max_x, max_y) \
    do {                                       \
        (_paper)->min.x = min_x;                   \
        (_paper)->min.y = min_y;                   \
        (_paper)->max.x = max_x;                   \
        (_paper)->max.y = max_y;                   \
    } while (0)

#define pen_reset(_paper, colr)                        \
    do {                                       \
        (_paper)->pen.width.x = DFL_PEN_WIDTH_X;       \
        (_paper)->pen.width.y = DFL_PEN_WIDTH_Y;       \
        (_paper)->pen.pos.x = (_paper)->padding.outer.x; \
        (_paper)->pen.pos.y = (_paper)->padding.outer.y; \
        (_paper)->pen.color = colr;                    \
    } while (0)
#define pen_pos(_paper, _x, _y)  \
    do {               \
        (_paper)->pen.pos.x = _x; \
        (_paper)->pen.pos.y = _y; \
    } while (0)

void paper_write(struct paper_s *paper, const char *text, int count);
void paper_print(struct paper_s *paper, const char *text, int align, int count);
void paper_printf(struct paper_s *paper, const char *fmt, ...);

#define pprintf(_paper, fmt, ...) \
    paper_printf((_paper), fmt, ##__VA_ARGS__);

#define pprintf_align(_paper, _align, fmt, ...) \
    do { \
        int _prev_align = (_paper)->align; \
        (_paper)->align = PAPER_ALIGN_##_align; \
        paper_printf(_paper, fmt, ##__VA_ARGS__); \
        (_paper)->align = _prev_align; \
    } while (0)

#define pprintf_color(_paper, _color, fmt, ...) \
    do { \
        uint32_t _prev_color = (_paper)->pen.color; \
        (_paper)->pen.color = _color; \
        paper_printf(_paper, fmt, ##__VA_ARGS__); \
        (_paper)->pen.color = _prev_color; \
    } while (0)

void paper_draw_rectangle(struct paper_s *paper, int x, int y, int width, int height, uint32_t color, int fill_pixels);

#endif /* __PAPER_H__ */
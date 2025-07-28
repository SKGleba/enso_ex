#include "paper.h"

struct pen_dets default_pen = {
	.pos = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
	.width = {DFL_PEN_WIDTH_X, DFL_PEN_WIDTH_Y},
	.color = DFL_PEN_COLR
};

struct paper_dets default_paper = {
	.min = {DFL_PAPER_START_X, DFL_PAPER_START_Y},
	.max = {DFL_PAPER_END_X, DFL_PAPER_END_Y},
	.color = DFL_PAPER_COLR,
	.pen = &default_pen,
    .blank_mode = DFL_PAPER_BLANK_MODE,
    .align = PAPER_ALIGN_LEFT,
    .padding = {
        .inner = {DFL_PAPER_IPAD_X, DFL_PAPER_IPAD_Y},
        .outer = {DFL_PAPER_OPAD_X, DFL_PAPER_OPAD_Y}
    }
};

extern uint8_t msx_font[];

static char *my_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return NULL;
}

static void paper_draw_char(struct paper_dets *paper, char c) {
    int i, j;
    uint8_t *glyph = &msx_font[c * 8];

    if (c < 0x20 || c > 0x7E)
        return;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            uint32_t c = paper->color;  // Default color is paper color

            if (*glyph & (128 >> j))
                c = paper->pen->color;

            draw_rectangle(paper->min.x + paper->pen->pos.x + 2 * j, paper->min.y + paper->pen->pos.y + 2 * i, 2, 2, c);
        }
        glyph++;
    }
}

void paper_write(struct paper_dets *paper, const char *text, int count) {
    while (count-- && *text) {
        if (*text != '\n') {
            paper_draw_char(paper, *text);
            paper->pen->pos.x += (paper->pen->width.x + paper->padding.inner.x);
        }
        if ((*text == '\n') || (paper->pen->pos.x + (2 * paper->pen->width.x) + paper->padding.inner.x > paper->max.x)) {
            paper->pen->pos.x = paper->padding.outer.x;
            paper->pen->pos.y += (paper->pen->width.y + paper->padding.inner.y);
            if (paper->min.y + paper->pen->pos.y + paper->padding.outer.y > paper->max.y) {
                paper->pen->pos.y = paper->padding.outer.y;
                if (paper->blank_mode & PAPER_BLANK_MODE_AREA_CLEAR)
                    draw_rectangle(paper->min.x, paper->min.y, paper->max.x - paper->min.x, paper->max.y - paper->min.y, paper->color);
            }
            if (paper->blank_mode & PAPER_BLANK_MODE_LINE_CLEAR) {
                draw_rectangle(paper->min.x, paper->min.y + paper->pen->pos.y, 
                               paper->max.x - paper->min.x, paper->pen->width.y,
                               paper->color);
            }
        }
        text++;
    }
}

void paper_print(struct paper_dets *paper, const char *text, int align, int count) {
    while (*text && count--) {
        const char *line_start = text;
        const char *line_end = my_strchr(line_start, '\n');

        if (!line_end)
            line_end = text + strlen(text);

        if (line_end > text + count)
            line_end = text + count;

        if (line_end != line_start) {
            int line_length = line_end - line_start;
            int line_width = line_length * (paper->pen->width.x + paper->padding.inner.x);

            if (align == PAPER_ALIGN_CENTER)
                paper->pen->pos.x = ((paper->max.x - paper->min.x) - line_width) / 2;
            else if (align == PAPER_ALIGN_RIGHT)
                paper->pen->pos.x = (paper->max.x - paper->min.x) - line_width;
            else
                paper->pen->pos.x = paper->padding.outer.x;

            paper_write(paper, line_start, line_length);
        }

        if (*line_end == '\n') {
            paper->pen->pos.y += (paper->pen->width.y + paper->padding.inner.y);
            if (paper->pen->pos.y + (2 * paper->pen->width.y) + paper->padding.inner.y > paper->max.y) {
                paper->pen->pos.y = paper->padding.outer.y;
                if (paper->blank_mode & PAPER_BLANK_MODE_AREA_CLEAR)
                    draw_rectangle(paper->min.x, paper->min.y, paper->max.x - paper->min.x, paper->max.y - paper->min.y, paper->color);
            }
            if (paper->blank_mode & PAPER_BLANK_MODE_LINE_CLEAR) {
                draw_rectangle(paper->min.x, paper->min.y + paper->pen->pos.y,
                               paper->max.x - paper->min.x, paper->pen->width.y,
                               paper->color);
            }
            text = line_end + 1;
        } else
            break;
    }
}

void paper_printf(struct paper_dets *paper, const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (paper->align != PAPER_ALIGN_LEFT)
        paper_print(paper, buffer, paper->align, sizeof(buffer) - 1);
    else
        paper_write(paper, buffer, sizeof(buffer) - 1); //og
}

void paper_draw_rectangle(struct paper_dets *paper, int x, int y, int width, int height, uint32_t color, int fill_pixels) {
    x = paper->min.x + x;
    y = paper->min.y + y;
    if ((width + x) > paper->max.x)
        width = paper->max.x - x;
    if ((height + y) > paper->max.y)
        height = paper->max.y - y;

    // Draw the rectangle
    if (fill_pixels <= 0)
        draw_rectangle(x, y, width, height, color);
    else {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if ((i < fill_pixels) || (i >= height - fill_pixels) ||
                    (j < fill_pixels) || (j >= width - fill_pixels)) {
                    draw_pixel(x + j, y + i, color);
                }
            }
        }
    }
}
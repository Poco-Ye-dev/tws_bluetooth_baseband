#ifndef __UTILS_UI_H__
#define __UTILS_UI_H__

#include <core/types.h>

/*
 * Minimal RGB565 graphics stack: a canvas with drawing primitives, a 5x7
 * font and a widget list that routes input to the first hit.
 */

#define UI_RGB(r, g, b) \
    ((u16)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

struct ui_rect {
    u16 x, y, w, h;
};

struct canvas {
    u16 *fb;
    u16 width;
    u16 height;
    u16 color;
};

struct ui;

typedef void (*ui_cb_t)(struct ui *ui, void *arg);

enum ui_widget_type {
    UI_WIDGET_LABEL = 0,
    UI_WIDGET_BUTTON,
};

struct ui_widget {
    struct ui_widget *next;
    enum ui_widget_type type;
    struct ui_rect rect;
    const char *text;
    u16 fg;
    u16 bg;
    bool pressed;
    ui_cb_t cb;
    void *arg;
};

struct ui {
    struct canvas canvas;
    struct ui_widget *widgets;
    struct ui_widget *focus;
};

void canvas_init(struct canvas *canvas, u16 *fb, u16 width, u16 height);
void canvas_clear(struct canvas *canvas, u16 color);
void canvas_pixel(struct canvas *canvas, int x, int y);
void canvas_fill_rect(struct canvas *canvas, struct ui_rect r);
void canvas_hline(struct canvas *canvas, int x, int y, int len);
void canvas_vline(struct canvas *canvas, int x, int y, int len);
void canvas_line(struct canvas *canvas, int x0, int y0, int x1, int y1);
void canvas_rect(struct canvas *canvas, struct ui_rect r);
void canvas_circle(struct canvas *canvas, int cx, int cy, int radius);
void canvas_blit(struct canvas *canvas, int x, int y,
                 const u16 *src, u16 w, u16 h);

void ui_init(struct ui *ui, u16 *fb, u16 width, u16 height);
struct ui_widget *ui_add_label(struct ui *ui, struct ui_rect rect,
                               const char *text, u16 fg, u16 bg);
struct ui_widget *ui_add_button(struct ui *ui, struct ui_rect rect,
                                const char *text, ui_cb_t cb, void *arg);
void ui_draw(struct ui *ui);
bool ui_touch(struct ui *ui, int x, int y, bool down);

#endif /* __UTILS_UI_H__ */

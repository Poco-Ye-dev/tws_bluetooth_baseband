#include <utils/ui.h>
#include <core/string.h>
#include <core/mem.h>

/*
 * Software renderer.  Everything writes directly into the framebuffer; the
 * widget layer only decides colours and ordering, which keeps the display
 * driver free of drawing knowledge.
 */

static const u8 font5x7[][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, /* space */
    { 0x3E, 0x51, 0x49, 0x45, 0x3E }, /* 0 */
    { 0x00, 0x42, 0x7F, 0x40, 0x00 }, /* 1 */
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, /* 2 */
    { 0x21, 0x41, 0x45, 0x4B, 0x31 }, /* 3 */
    { 0x18, 0x14, 0x12, 0x7F, 0x10 }, /* 4 */
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, /* 5 */
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, /* 6 */
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, /* 7 */
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, /* 8 */
    { 0x06, 0x49, 0x49, 0x29, 0x1E }, /* 9 */
    { 0x7E, 0x11, 0x11, 0x11, 0x7E }, /* A */
    { 0x7F, 0x49, 0x49, 0x49, 0x36 }, /* B */
    { 0x3E, 0x41, 0x41, 0x41, 0x22 }, /* C */
    { 0x7F, 0x41, 0x41, 0x22, 0x1C }, /* D */
    { 0x7F, 0x49, 0x49, 0x49, 0x41 }, /* E */
    { 0x7F, 0x09, 0x09, 0x09, 0x01 }, /* F */
    { 0x3E, 0x41, 0x49, 0x49, 0x7A }, /* G */
    { 0x7F, 0x08, 0x08, 0x08, 0x7F }, /* H */
    { 0x00, 0x41, 0x7F, 0x41, 0x00 }, /* I */
    { 0x20, 0x40, 0x41, 0x3F, 0x01 }, /* J */
    { 0x7F, 0x08, 0x14, 0x22, 0x41 }, /* K */
    { 0x7F, 0x40, 0x40, 0x40, 0x40 }, /* L */
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, /* M */
    { 0x7F, 0x04, 0x08, 0x10, 0x7F }, /* N */
    { 0x3E, 0x41, 0x41, 0x41, 0x3E }, /* O */
    { 0x7F, 0x09, 0x09, 0x09, 0x06 }, /* P */
    { 0x3E, 0x41, 0x51, 0x21, 0x5E }, /* Q */
    { 0x7F, 0x09, 0x19, 0x29, 0x46 }, /* R */
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, /* S */
    { 0x01, 0x01, 0x7F, 0x01, 0x01 }, /* T */
    { 0x3F, 0x40, 0x40, 0x40, 0x3F }, /* U */
    { 0x1F, 0x20, 0x40, 0x20, 0x1F }, /* V */
    { 0x3F, 0x40, 0x38, 0x40, 0x3F }, /* W */
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, /* X */
    { 0x07, 0x08, 0x08, 0x08, 0x07 }, /* Y */
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, /* Z */
};

static int glyph_index(char c)
{
    if (c >= '0' && c <= '9')
        return 1 + (c - '0');
    if (c >= 'A' && c <= 'Z')
        return 11 + (c - 'A');
    return 0;
}

void canvas_init(struct canvas *canvas, u16 *fb, u16 width, u16 height)
{
    canvas->fb = fb;
    canvas->width = width;
    canvas->height = height;
    canvas->color = UI_RGB(255, 255, 255);
}

void canvas_pixel(struct canvas *canvas, int x, int y)
{
    if (x < 0 || y < 0 || x >= canvas->width || y >= canvas->height)
        return;
    canvas->fb[y * canvas->width + x] = canvas->color;
}

void canvas_clear(struct canvas *canvas, u16 color)
{
    for (u32 i = 0; i < (u32)canvas->width * canvas->height; i++)
        canvas->fb[i] = color;
}

void canvas_hline(struct canvas *canvas, int x, int y, int len)
{
    for (int i = 0; i < len; i++)
        canvas_pixel(canvas, x + i, y);
}

void canvas_vline(struct canvas *canvas, int x, int y, int len)
{
    for (int i = 0; i < len; i++)
        canvas_pixel(canvas, x, y + i);
}

void canvas_fill_rect(struct canvas *canvas, struct ui_rect r)
{
    for (int y = r.y; y < r.y + r.h; y++)
        canvas_hline(canvas, r.x, y, r.w);
}

void canvas_rect(struct canvas *canvas, struct ui_rect r)
{
    canvas_hline(canvas, r.x, r.y, r.w);
    canvas_hline(canvas, r.x, r.y + r.h - 1, r.w);
    canvas_vline(canvas, r.x, r.y, r.h);
    canvas_vline(canvas, r.x + r.w - 1, r.y, r.h);
}

void canvas_line(struct canvas *canvas, int x0, int y0, int x1, int y1)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        canvas_pixel(canvas, x0, y0);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void canvas_circle(struct canvas *canvas, int cx, int cy, int radius)
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        canvas_pixel(canvas, cx + x, cy + y);
        canvas_pixel(canvas, cx + y, cy + x);
        canvas_pixel(canvas, cx - y, cy + x);
        canvas_pixel(canvas, cx - x, cy + y);
        canvas_pixel(canvas, cx - x, cy - y);
        canvas_pixel(canvas, cx - y, cy - x);
        canvas_pixel(canvas, cx + y, cy - x);
        canvas_pixel(canvas, cx + x, cy - y);

        y++;
        if (err <= 0)
            err += 2 * y + 1;
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

void canvas_blit(struct canvas *canvas, int x, int y,
                 const u16 *src, u16 w, u16 h)
{
    for (u16 row = 0; row < h; row++) {
        for (u16 col = 0; col < w; col++) {
            canvas->color = src[row * w + col];
            canvas_pixel(canvas, x + col, y + row);
        }
    }
}

static void draw_text(struct canvas *canvas, int x, int y, const char *text,
                      u16 fg, u16 scale)
{
    for (; *text; text++) {
        const u8 *glyph = font5x7[glyph_index(*text)];
        for (int col = 0; col < 5; col++) {
            for (int row = 0; row < 7; row++) {
                if (!(glyph[col] & (1u << row)))
                    continue;
                canvas->color = fg;
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        canvas_pixel(canvas, x + col * scale + sx,
                                     y + row * scale + sy);
            }
        }
        x += 6 * scale;
    }
}

void ui_init(struct ui *ui, u16 *fb, u16 width, u16 height)
{
    memset(ui, 0, sizeof(*ui));
    canvas_init(&ui->canvas, fb, width, height);
}

static struct ui_widget *widget_new(struct ui *ui, enum ui_widget_type type,
                                    struct ui_rect rect, const char *text,
                                    u16 fg, u16 bg)
{
    struct ui_widget *widget = kzalloc(sizeof(*widget));
    if (!widget)
        return NULL;

    widget->type = type;
    widget->rect = rect;
    widget->text = text;
    widget->fg = fg;
    widget->bg = bg;
    widget->next = ui->widgets;
    ui->widgets = widget;
    return widget;
}

struct ui_widget *ui_add_label(struct ui *ui, struct ui_rect rect,
                               const char *text, u16 fg, u16 bg)
{
    return widget_new(ui, UI_WIDGET_LABEL, rect, text, fg, bg);
}

struct ui_widget *ui_add_button(struct ui *ui, struct ui_rect rect,
                                const char *text, ui_cb_t cb, void *arg)
{
    struct ui_widget *widget = widget_new(ui, UI_WIDGET_BUTTON, rect, text,
                                          UI_RGB(255, 255, 255),
                                          UI_RGB(40, 90, 180));
    if (widget) {
        widget->cb = cb;
        widget->arg = arg;
    }
    return widget;
}

void ui_draw(struct ui *ui)
{
    for (struct ui_widget *w = ui->widgets; w; w = w->next) {
        ui->canvas.color = w->pressed ? w->fg : w->bg;
        canvas_fill_rect(&ui->canvas, w->rect);
        ui->canvas.color = UI_RGB(255, 255, 255);
        canvas_rect(&ui->canvas, w->rect);
        draw_text(&ui->canvas, w->rect.x + 6, w->rect.y + 6, w->text,
                  w->pressed ? w->bg : w->fg, 1);
    }
}

bool ui_touch(struct ui *ui, int x, int y, bool down)
{
    for (struct ui_widget *w = ui->widgets; w; w = w->next) {
        if (w->type != UI_WIDGET_BUTTON)
            continue;
        if (x < w->rect.x || y < w->rect.y ||
            x >= w->rect.x + w->rect.w || y >= w->rect.y + w->rect.h)
            continue;

        if (down) {
            w->pressed = true;
            if (w->cb)
                w->cb(ui, w->arg);
            return true;
        }
        w->pressed = false;
        return true;
    }
    return false;
}

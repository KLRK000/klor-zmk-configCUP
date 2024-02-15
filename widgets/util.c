#include <zephyr/kernel.h>
#include "util.h"
#include "widgets/lv_canvas.h"

lv_color_t k_foreground() { return lv_color_black(); }

lv_color_t k_background() { return lv_color_white(); }

lv_color_t k_label_color() { return lv_color_make(0x00, 0x00, 0xff); }

void draw_battery(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_rect_dsc_t rect_blue_dsc;
    init_rect_desc(&rect_blue_dsc, k_foreground());
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_desc(&rect_black_dsc, k_background());

    lv_canvas_draw_rect(canvas, 0, 2, 29, 12, &rect_blue_dsc);
    lv_canvas_draw_rect(canvas, 1, 3, 27, 10, &rect_black_dsc);
    lv_canvas_draw_rect(canvas, 2, 4, (state->battery + 2) >> 2, 8, &rect_blue_dsc);
    lv_canvas_draw_rect(canvas, 30, 5, 3, 6, &rect_blue_dsc);

    if (state->charging) {
        lv_draw_label_dsc_t desc;
        init_label_dsc(&desc, k_foreground(), &lv_font_montserrat_12, LV_TEXT_ALIGN_LEFT);
        lv_canvas_draw_text(canvas, 39, 1, 25, &desc, LV_SYMBOL_CHARGE);
    }
}

void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align) {
    lv_draw_label_dsc_init(label_dsc);
    label_dsc->color = color;
    label_dsc->font = font;
    label_dsc->align = align;
}

void init_rect_desc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color) {
    lv_draw_rect_dsc_init(rect_dsc);
    rect_dsc->bg_color = bg_color;
}
#pragma once

#include <zephyr/kernel.h>
#include "util.h"

#define LABEL_CANVAS_HEIGHT 14

struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    // top canvas buffer
    lv_color_t cbuf[CANVAS_SIZE * CANVAS_SIZE];
    struct status_state state;
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget);
#pragma once

#include <zephyr/kernel.h>
#include "util.h"

#define BONGO_CAT_IDLE_LIMIT 30
#define BONGO_CAT_SLOW_LIMIT 60
#define LABEL_CANVAS_HEIGHT 14

struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    // top canvas buffer
    lv_color_t cbuf[CANVAS_SIZE * CANVAS_SIZE];
    // layer canvas buffer
    lv_color_t cbuf2[CANVAS_SIZE * CANVAS_SIZE];
    // cat animation buffer
    lv_color_t cbuf3[CANVAS_SIZE * CANVAS_SIZE];
    struct status_state state;
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget);
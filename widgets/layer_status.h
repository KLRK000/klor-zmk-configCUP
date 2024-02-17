
#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_layer_status_widget {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_layer_status_widget_init(struct zmk_layer_status_widget *widget,lv_obj_t *parent);
lv_obj_t *zmk_layer_status_widget_obj(struct zmk_layer_status_widget *widget);
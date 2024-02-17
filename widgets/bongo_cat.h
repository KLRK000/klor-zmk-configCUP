

#include <zephyr/kernel.h>
#include <lvgl.h>

#define BONGO_CAT_IDLE_LIMIT 30
#define BONGO_CAT_SLOW_LIMIT 60

struct zmk_bongo_cat_widget {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_anim_t anim;
};

int zmk_bongo_cat_widget_init(struct zmk_bongo_cat_widget *widget, lv_obj_t *parent);
lv_obj_t *zmk_bongo_cat_widget_obj(struct zmk_bongo_cat_widget *widget);
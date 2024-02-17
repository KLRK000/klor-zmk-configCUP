#include <zephyr/logging/log.h>
#include <lvgl.h>
#include "widgets/status.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include "widgets/bongo_cat.h"
#include "widgets/layer_status.h"

static struct zmk_bongo_cat_widget bongo_cat_widget;
static struct zmk_layer_status_widget layer_status_widget;

#endif

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
static struct zmk_widget_status status_widget;
#endif

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;

    screen = lv_obj_create(NULL);

#if IS_ENABLED(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
    zmk_widget_status_init(&status_widget, screen);
    lv_obj_align(zmk_widget_status_obj(&status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zmk_bongo_cat_widget_init(&bongo_cat_widget, screen);

    zmk_layer_status_widget_init(&layer_status_widget, screen);
    lv_obj_align(zmk_layer_status_widget_obj(&layer_status_widget), LV_ALIGN_BOTTOM_RIGHT, 0, 0);

#endif

    return screen;
}
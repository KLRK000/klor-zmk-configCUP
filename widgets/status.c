#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>
#include "zephyr/logging/log.h"
#include "zephyr/sys/slist.h"
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "status.h"
#include <lvgl.h>
#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/ble.h>
#include <zmk/usb.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

enum anim_state {
    anim_state_none,
    anim_state_idle,
    anim_state_slow,
    anim_state_fast,
} current_anim_state;

const void **images;

LV_IMG_DECLARE(idle_img1);
LV_IMG_DECLARE(idle_img2);
LV_IMG_DECLARE(idle_img3);
LV_IMG_DECLARE(idle_img4);
LV_IMG_DECLARE(idle_img5);

// no animation state
LV_IMG_DECLARE(slow_img);

// fast cat typing images.
LV_IMG_DECLARE(fast_img1);
LV_IMG_DECLARE(fast_img2);

// clang-format off
const void *idle_images[] = {
    &idle_img1,
    &idle_img2,
    &idle_img3,
    &idle_img4,
    &idle_img5,
};
// clang-format on
const void *fast_images[] = {
    &fast_img1,
    &fast_img2,
};

static void set_img_src(void *var, int32_t val) {
    lv_obj_t *img = (lv_obj_t *)var;
    lv_img_set_src(img, images[val]);
}

struct layer_status_state {
    uint8_t index;
    const char *label;
};

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};

struct wpm_status_state {
    uint8_t wpm;
};

static void draw_top(lv_obj_t *widget, const struct status_state *state) {
    // get the canvas from object.
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    // background rect
    lv_draw_rect_dsc_t background_rect_desc;
    init_rect_desc(&background_rect_desc, k_background());

    // description for bluetooth connection symbol
    lv_draw_label_dsc_t label_desc;
    init_label_dsc(&label_desc, k_foreground(), &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);

    // clear the canvas with black (fill background)
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &background_rect_desc);

    draw_battery(canvas, state);

    char text[20] = {};

    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        strcat(text, LV_SYMBOL_USB);
        break;
    case ZMK_TRANSPORT_BLE:
        if (state->active_profile_bonded) {
            if (state->active_profile_connected) {
                snprintf(text, sizeof(text), LV_SYMBOL_WIFI " %i " LV_SYMBOL_OK,
                         state->selected_endpoint.ble.profile_index + 1);
            } else {
                snprintf(text, sizeof(text), LV_SYMBOL_WIFI " %i " LV_SYMBOL_CLOSE,
                         state->selected_endpoint.ble.profile_index + 1);
            }
        } else {
            snprintf(text, sizeof(text), LV_SYMBOL_WIFI " %i " LV_SYMBOL_SETTINGS,
                     state->selected_endpoint.ble.profile_index + 1);
        }
        break;
    }

    lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_desc, text);
}

void draw_layer_status(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *label = lv_obj_get_child(widget, -2);

    if (state->layer_label == NULL) {
        char text[19] = {};

        sprintf(text, "#0000ff LAYER %i#", state->layer_index);

        lv_label_set_text(label, text);
    } else {
        char text[19] = {};

        snprintf(text, sizeof(text), "#0000ff %s#", state->layer_label);

        lv_label_set_text(label, text);
    }
}

void draw_wpm_status(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *label = lv_obj_get_child(widget, -1);

    char text[13] = {};

    sprintf(text, "#0000ff %i#", state->wpm);

    lv_label_set_text(label, text);
}

void draw_cat_animation(lv_obj_t *widget, struct status_state *state) {
    lv_obj_t *img = lv_obj_get_child(widget, 1);

    if (state->wpm < BONGO_CAT_IDLE_LIMIT) {
        if (current_anim_state != anim_state_idle) {
            // show the idle animation
            lv_anim_init(&state->anim);
            lv_anim_set_var(&state->anim, img);
            lv_anim_set_time(&state->anim, 1000);
            lv_anim_set_repeat_delay(&state->anim, 100);
            lv_anim_set_values(&state->anim, 0, 4);
            lv_anim_set_exec_cb(&state->anim, set_img_src);
            lv_anim_set_repeat_count(&state->anim, 10);
            images = idle_images;
            current_anim_state = anim_state_idle;
            lv_anim_start(&state->anim);
        }
    } else if (state->wpm < BONGO_CAT_SLOW_LIMIT) {
        if (current_anim_state != anim_state_slow) {
            lv_anim_del(widget, set_img_src);
            lv_img_set_src(widget, &slow_img);
            current_anim_state = anim_state_slow;
        }
    } else {
        if (current_anim_state != anim_state_fast) {
            // show the fast typing animation
            lv_anim_init(&state->anim);
            lv_anim_set_var(&state->anim, img);
            lv_anim_set_time(&state->anim, 500);
            lv_anim_set_repeat_delay(&state->anim, 500);
            lv_anim_set_values(&state->anim, 0, 1);
            lv_anim_set_exec_cb(&state->anim, set_img_src);
            lv_anim_set_repeat_count(&state->anim, LV_ANIM_REPEAT_INFINITE);
            images = fast_images;
            current_anim_state = anim_state_fast;

            lv_anim_start(&state->anim);
        }
    }
}

/// BATTERY LISTENER FUNCTIONS

static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;
    draw_top(widget->obj, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state) {
        .level = zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

/// BATTERY STATUS END

/// OUTPUT STATUS START

static void set_output_status(struct zmk_widget_status *widget, struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw_top(widget->obj, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)

ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

/// OUTPUT STATUT END

/// LAYER UPDATE START

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;

    draw_layer_status(widget->obj, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); };
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t layer_index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = layer_index,
        .label = zmk_keymap_layer_name(layer_index),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

/// LAYER STATUS END

/// WPM START

static void set_wpm_status(struct wpm_status_state state, struct zmk_widget_status *widget) {
    widget->state.wpm = state.wpm;

    draw_cat_animation(widget->obj, &widget->state);
    draw_wpm_status(widget->obj, &widget->state);
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(state, widget); }
}

static struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    return (struct wpm_status_state){.wpm = zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)

ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

/// WPM END

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->state.wpm = 0;
    // create a screen
    widget->obj = lv_obj_create(parent);
    // configure the size of display
    lv_obj_set_size(widget->obj, 128, 64);

    // create top header canvas
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_ALPHA_1BIT);

    lv_obj_t *img = lv_img_create(widget->obj);
    lv_obj_align(img, LV_ALIGN_BOTTOM_MID, 0, 0);

    /// layer update canvas
    lv_obj_t *layer_status = lv_label_create(widget->obj);
    lv_obj_align(layer_status, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_label_set_recolor(layer_status, true);

    lv_obj_t *wpm_status = lv_label_create(widget->obj);
    lv_obj_align(wpm_status, LV_ALIGN_TOP_LEFT, 0, 17);
    lv_label_set_recolor(wpm_status, true);

    sys_slist_append(&widgets, &widget->node);

    widget_battery_status_init();
    widget_output_status_init();
    widget_layer_status_init();
    widget_wpm_status_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
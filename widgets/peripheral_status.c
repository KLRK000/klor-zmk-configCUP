#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>

#include "zephyr/logging/log.h"
#include "zephyr/sys/slist.h"
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>

#include "peripheral_status.h"

LV_IMG_DECLARE(batman_logo);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct peripheral_state_status {
    bool connected;
};

void draw_top(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    lv_draw_rect_dsc_t background_rect_desc;
    init_rect_desc(&background_rect_desc, lv_color_white());

    lv_draw_label_dsc_t connection_state_label;
    init_label_dsc(&connection_state_label, k_foreground(), &lv_font_montserrat_16,
                   LV_ALIGN_TOP_RIGHT);

    // fill the background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &background_rect_desc);

    draw_battery(canvas, state);

    const char *text =
        state->connected ? (LV_SYMBOL_WIFI " " LV_SYMBOL_OK) : (LV_SYMBOL_WIFI " " LV_SYMBOL_CLOSE);

    lv_canvas_draw_text(canvas, -2, 0, CANVAS_SIZE, &connection_state_label, text);
}

/// BATTERY

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
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

/// BATTERY END

/// CONNECTION START

static void set_connection_status(struct zmk_widget_status *widget,
                                  struct peripheral_state_status state) {
    widget->state.connected = state.connected;

    draw_top(widget->obj, &widget->state);
}

static struct peripheral_state_status get_state(const zmk_event_t *eh) {
    return (struct peripheral_state_status){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void connection_state_update_cb(struct peripheral_state_status state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_state_status,
                            connection_state_update_cb, get_state)

ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

/// CONNECTION END

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_ALPHA_1BIT);

    // batman logo
    lv_obj_t *logo = lv_img_create(widget->obj);
    lv_img_set_src(logo, &batman_logo);
    lv_obj_align(logo, LV_ALIGN_BOTTOM_MID, 0, 0);

    sys_slist_append(&widgets, &widget->node);

    // Intialization
    widget_battery_status_init();
    widget_peripheral_status_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }

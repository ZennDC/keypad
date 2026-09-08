#include "matrix.h"
#include "config_manager.h"
#include "protocol.h"
#include "pico/stdlib.h"
#include "tusb.h"

#define DEBOUNCE_MS 15

static const uint8_t key_pins[NUM_MACRO_KEYS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static uint32_t last_scan_time = 0;

void matrix_init(void) {
    for (size_t i = 0; i < NUM_MACRO_KEYS; i++) {
        gpio_init(key_pins[i]);
        gpio_set_dir(key_pins[i], GPIO_IN);
        gpio_pull_up(key_pins[i]);
    }
}

void matrix_task(void) {
    if (!tud_hid_ready()) return;

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_scan_time < DEBOUNCE_MS) return;

    macro_key_t *keys = config_manager_get_keys();

    for (size_t i = 0; i < NUM_MACRO_KEYS; i++) {
        bool is_pressed = !gpio_get(key_pins[i]);

        if (is_pressed && !keys[i].was_pressed) {
            if (keys[i].report_type == 0x02) {
                tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &keys[i].usage_code, sizeof(uint16_t));
            } else {
                uint8_t keycode[6] = {(uint8_t)keys[i].usage_code, 0, 0, 0, 0, 0};
                tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
            }
            keys[i].was_pressed = true;
            last_scan_time = now;
            break;
        } else if (!is_pressed && keys[i].was_pressed) {
            if (keys[i].report_type == 0x02) {
                uint16_t empty = 0;
                tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty, sizeof(empty));
            } else {
                tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
            }
            keys[i].was_pressed = false;
            last_scan_time = now;
            break;
        }
    }
}
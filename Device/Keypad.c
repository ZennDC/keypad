#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

#define TFT_BL_PIN 22 // ST7789 Backlight PWM GPIO

st7789_config_t display_cfg = {
    .mode = 0,
    .orientation = 0,
    .brightness = 80};

// Switch definitions & default actions
macro_key_t keys[16] = {
    {.report_type = 0x02, .usage_code = HID_USAGE_CONSUMER_VOLUME_INCREMENT, .was_pressed = false},
    {.report_type = 0x02, .usage_code = HID_USAGE_CONSUMER_VOLUME_DECREMENT, .was_pressed = false},
    {.report_type = 0x02, .usage_code = HID_USAGE_CONSUMER_PLAY_PAUSE, .was_pressed = false},
    {.report_type = 0x02, .usage_code = HID_USAGE_CONSUMER_SCAN_NEXT, .was_pressed = false},
    {.report_type = 0x02, .usage_code = HID_USAGE_CONSUMER_SCAN_PREVIOUS, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_ENTER, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_ESCAPE, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_TAB, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_SPACE, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_C, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_V, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_Z, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_F13, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_F14, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_F15, .was_pressed = false},
    {.report_type = 0x01, .usage_code = HID_KEY_F16, .was_pressed = false},
};

void set_display_brightness(uint8_t percent)
{
    if (percent > 100)
        percent = 100;
    uint slice_num = pwm_gpio_to_slice_num(TFT_BL_PIN);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(TFT_BL_PIN), (percent * 255) / 100);
}

enum {
    CMD_SAVE_TO_FLASH = 0xAA // Host tells the device: "Done editing, commit to flash"
};

void handle_keypad_config_packet(uint8_t const *buffer, uint16_t bufsize)
{
    if (bufsize < 1) return;

    uint8_t target = buffer[0];

    // Explicit save trigger
    if (target == CMD_SAVE_TO_FLASH) {
        flash_storage_save();
        return;
    }

    if (bufsize < 4) return;

    // Display configuration: [0xFF, mode, orientation, brightness]
    if (target == 0xFF) {
        display_cfg.mode = buffer[1];
        display_cfg.orientation = buffer[2];
        display_cfg.brightness = buffer[3];
        set_display_brightness(display_cfg.brightness);
        return;
    }

    // Key configuration: [key_idx, report_type, usage_lsb, usage_msb]
    if (target < 16) {
        keys[target].report_type = buffer[1];
        keys[target].usage_code  = (uint16_t)(buffer[2] | (buffer[3] << 8));
    }
}

static void init_backlight_pwm(void)
{
    gpio_set_function(TFT_BL_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(TFT_BL_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 255);
    pwm_init(slice_num, &config, true);
    set_display_brightness(display_cfg.brightness);
}

int main(void)
{
    board_init();
    tusb_init();
    init_backlight_pwm();

    // for (size_t i = 0; i < 16; i++)
    // {
    //     gpio_init(keys[i].btn);
    //     gpio_set_dir(keys[i].btn, GPIO_IN);
    //     gpio_pull_up(keys[i].btn);
    // }

    flash_storage_init();

    while (1)
    {
        tud_task();

        if (!tud_hid_ready())
            continue;

        for (size_t i = 0; i < 16; i++)
        {
            bool is_pressed = false; // !gpio_get(keys[i].btn);

            if (is_pressed && !keys[i].was_pressed)
            {
                if (keys[i].report_type == 0x02)
                {
                    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &keys[i].usage_code, sizeof(uint16_t));
                }
                else
                {
                    uint8_t keycode[6] = {(uint8_t)keys[i].usage_code, 0, 0, 0, 0, 0};
                    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
                }
                keys[i].was_pressed = true;
                sleep_ms(15);
                break;
            }
            else if (!is_pressed && keys[i].was_pressed)
            {
                if (keys[i].report_type == 0x02)
                {
                    uint16_t empty = 0;
                    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty, sizeof(empty));
                }
                else
                {
                    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
                }
                keys[i].was_pressed = false;
                sleep_ms(15);
                break;
            }
        }
    }

    return 0;
}

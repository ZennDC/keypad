#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"

#define PIN_VOLUME_UP 2
#define PIN_VOLUME_DOWN 3
#define PIN_PLAY_PAUSE 4
#define PIN_NEXT 5
#define PIN_PREV 6

enum
{
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_CONSUMER_CONTROL = 2,
};

typedef struct
{
    uint pin;
    uint16_t usage;
    bool was_pressed;
    uint32_t last_repeat_ms;
} button_t;

int main(void)
{
    board_init();
    tusb_init();

    button_t buttons[] = {
        {.pin = PIN_VOLUME_UP, .usage = HID_USAGE_CONSUMER_VOLUME_INCREMENT},
        {.pin = PIN_VOLUME_DOWN, .usage = HID_USAGE_CONSUMER_VOLUME_DECREMENT},
        {.pin = PIN_PLAY_PAUSE, .usage = HID_USAGE_CONSUMER_PLAY_PAUSE},
        {.pin = PIN_NEXT, .usage = HID_USAGE_CONSUMER_SCAN_NEXT},
        {.pin = PIN_PREV, .usage = HID_USAGE_CONSUMER_SCAN_PREVIOUS}};

    const size_t num_buttons = sizeof(buttons) / sizeof(buttons[0]);

    for (size_t i = 0; i < num_buttons; i++)
    {
        gpio_init(buttons[i].pin);
        gpio_set_dir(buttons[i].pin, GPIO_IN);
        gpio_pull_up(buttons[i].pin);
        buttons[i].was_pressed = false;
        buttons[i].last_repeat_ms = 0;
    }

    while (1)
    {
        tud_task();

        if (!tud_hid_ready())
            continue;

        uint32_t now = to_ms_since_boot(get_absolute_time());

        for (size_t i = 0; i < num_buttons; i++)
        {
            bool is_pressed = !gpio_get(buttons[i].pin);

            if (is_pressed && !buttons[i].was_pressed)
            {
                tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &buttons[i].usage, sizeof(buttons[i].usage));
                buttons[i].was_pressed = true;
                buttons[i].last_repeat_ms = now;
                sleep_ms(20);
                break;
            }
            else if (!is_pressed && buttons[i].was_pressed)
            {
                uint16_t empty = 0;
                tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty, sizeof(empty));
                buttons[i].was_pressed = false;
                sleep_ms(20);
                break;
            }
        }
    }

    return 0;
}
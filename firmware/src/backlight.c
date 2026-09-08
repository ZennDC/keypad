#include "backlight.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define TFT_BL_PIN 22

void backlight_init(uint8_t initial_brightness) {
    gpio_set_function(TFT_BL_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(TFT_BL_PIN);
    
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 255);
    pwm_init(slice_num, &config, true);
    
    backlight_set_brightness(initial_brightness);
}

void backlight_set_brightness(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint slice_num = pwm_gpio_to_slice_num(TFT_BL_PIN);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(TFT_BL_PIN), (percent * 255) / 100);
}
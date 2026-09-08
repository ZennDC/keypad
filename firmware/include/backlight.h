#ifndef BACKLIGHT_H_
#define BACKLIGHT_H_

#include <stdint.h>

void backlight_init(uint8_t initial_brightness);
void backlight_set_brightness(uint8_t percent);

#endif // BACKLIGHT_H_
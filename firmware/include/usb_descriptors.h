#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "protocol.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern st7789_config_t display_cfg;
extern macro_key_t     keys[NUM_MACRO_KEYS];

void set_display_brightness(uint8_t percent);
void handle_keypad_config_packet(uint8_t const *buffer, uint16_t bufsize);

#ifdef __cplusplus
}
#endif

#endif // USB_DESCRIPTORS_H_
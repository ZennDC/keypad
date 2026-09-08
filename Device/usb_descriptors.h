#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REPORT_ID_KEYBOARD         = 1,
    REPORT_ID_CONSUMER_CONTROL = 2,
    REPORT_ID_CONFIG_FEATURE   = 3,
};

// ST7789 display runtime configuration
typedef struct {
    uint8_t mode;        // 0: HW Monitor, 1: Profile, 2: Image, etc.
    uint8_t orientation; // 0: Portrait, 1: Landscape, etc.
    uint8_t brightness;  // 0 - 100%
} st7789_config_t;

// Macro key runtime configuration
typedef struct {
    uint8_t  report_type; // 0x01: Keyboard, 0x02: Consumer
    uint16_t usage_code;
    bool     was_pressed;
} macro_key_t;

extern st7789_config_t display_cfg;
extern macro_key_t     keys[16];

void set_display_brightness(uint8_t percent);
void handle_keypad_config_packet(uint8_t const *buffer, uint16_t bufsize);

typedef struct __attribute__((aligned(4))) {
    uint32_t        magic;
    uint8_t         version;
    st7789_config_t display_cfg;
    macro_key_t     keys[16];
    uint8_t         checksum; // Simple 8-bit sum to detect write truncations
} persistent_config_t;

void flash_storage_init(void);
void flash_storage_save(void);

#ifdef __cplusplus
}
#endif
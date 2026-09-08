#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_REPORT_SIZE          64
#define NUM_MACRO_KEYS              16

enum {
    REPORT_ID_KEYBOARD         = 1,
    REPORT_ID_CONSUMER_CONTROL = 2,
    REPORT_ID_CONFIG_FEATURE   = 3,
};

enum {
    CMD_CONFIG_TARGET_DISPLAY = 0xFF,
    CMD_SAVE_TO_FLASH         = 0xAA,
};

typedef struct __attribute__((packed)) {
    uint8_t mode;        // 0: HW Monitor, 1: Profile, 2: Image, etc.
    uint8_t orientation; // 0: Portrait, 1: Landscape, etc.
    uint8_t brightness;  // 0 - 100%
} st7789_config_t;

typedef struct __attribute__((packed)) {
    uint8_t  report_type; // 0x01: Keyboard, 0x02: Consumer
    uint16_t usage_code;
    bool     was_pressed;
} macro_key_t;

typedef struct __attribute__((aligned(4))) {
    uint32_t        magic;
    uint8_t         version;
    st7789_config_t display_cfg;
    macro_key_t     keys[NUM_MACRO_KEYS];
    uint8_t         checksum;
} persistent_config_t;

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_H_
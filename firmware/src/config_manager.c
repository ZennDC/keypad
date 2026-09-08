#include "config_manager.h"
#include "flash_storage.h"
#include "backlight.h"
#include "tusb.h"
#include <string.h>

static st7789_config_t display_cfg;
static macro_key_t keys[NUM_MACRO_KEYS];

static const persistent_config_t default_config = {
    .magic = FLASH_CONFIG_MAGIC,
    .version = FLASH_CONFIG_VERSION,
    .display_cfg = { .mode = 0, .orientation = 0, .brightness = 80 },
    .keys = {
        {0x02, HID_USAGE_CONSUMER_VOLUME_INCREMENT, false},
        {0x02, HID_USAGE_CONSUMER_VOLUME_DECREMENT, false},
        {0x02, HID_USAGE_CONSUMER_PLAY_PAUSE,        false},
        {0x02, HID_USAGE_CONSUMER_SCAN_NEXT,         false},
        {0x02, HID_USAGE_CONSUMER_SCAN_PREVIOUS,     false},
        {0x01, HID_KEY_ENTER,                       false},
        {0x01, HID_KEY_ESCAPE,                      false},
        {0x01, HID_KEY_TAB,                         false},
        {0x01, HID_KEY_SPACE,                       false},
        {0x01, HID_KEY_C,                           false},
        {0x01, HID_KEY_V,                           false},
        {0x01, HID_KEY_Z,                           false},
        {0x01, HID_KEY_F13,                         false},
        {0x01, HID_KEY_F14,                         false},
        {0x01, HID_KEY_F15,                         false},
        {0x01, HID_KEY_F16,                         false},
    }
};

void config_manager_init(void) {
    flash_storage_init();
    persistent_config_t stored;

    if (flash_storage_read(&stored)) {
        display_cfg = stored.display_cfg;
        memcpy(keys, stored.keys, sizeof(keys));
    } else {
        display_cfg = default_config.display_cfg;
        memcpy(keys, default_config.keys, sizeof(keys));
        flash_storage_write(&default_config);
    }
}

st7789_config_t *config_manager_get_display(void) {
    return &display_cfg;
}

macro_key_t *config_manager_get_keys(void) {
    return keys;
}

void config_manager_handle_packet(const uint8_t *buffer, uint16_t bufsize) {
    if (bufsize < 1) return;

    uint8_t target = buffer[0];

    if (target == CMD_SAVE_TO_FLASH) {
        persistent_config_t to_save = {
            .magic = FLASH_CONFIG_MAGIC,
            .version = FLASH_CONFIG_VERSION,
            .display_cfg = display_cfg
        };
        memcpy(to_save.keys, keys, sizeof(keys));
        flash_storage_write(&to_save);
        return;
    }

    if (bufsize < 4) return;

    if (target == CMD_CONFIG_TARGET_DISPLAY) {
        display_cfg.mode = buffer[1];
        display_cfg.orientation = buffer[2];
        display_cfg.brightness = buffer[3];
        backlight_set_brightness(display_cfg.brightness);
        return;
    }

    if (target < NUM_MACRO_KEYS) {
        keys[target].report_type = buffer[1];
        keys[target].usage_code  = (uint16_t)(buffer[2] | (buffer[3] << 8));
    }
}
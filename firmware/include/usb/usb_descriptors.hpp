#pragma once

#include "protocol.hpp"
#include <cstdint>
#include <span>

namespace zdc::usb {

// Host keyboard LED indicator flags (matches TinyUSB KEYBOARD_LED_* / HID spec)
enum LedIndicator : uint8_t {
    NumLock = 1 << 0,
    CapsLock = 1 << 1,
    ScrollLock = 1 << 2,
    Compose = 1 << 3,
    Kana = 1 << 4,
};

// Returns the current bitmask of active LED indicators (thread-safe)
uint8_t get_led_state();

// Sets the LED state bitmask (called from tud_hid_set_report_cb)
void set_led_state(uint8_t leds);

// Handler called when a Report ID 3 (Config Feature Report) arrives via TinyUSB
void handle_config_packet(std::span<const uint8_t> packet);

} // namespace zdc::usb

// ---------------------------------------------------------------------------
// C linkage declarations for TinyUSB device callbacks (tud_hid_*)
// ---------------------------------------------------------------------------
extern "C" {

// C-compatible packet router for tud_hid_set_report_cb
void handle_keypad_config_packet(const uint8_t* buffer, uint16_t bufsize);
}
#pragma once

#include "protocol.hpp"
#include <cstdint>
#include <span>

namespace zdc::usb {

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
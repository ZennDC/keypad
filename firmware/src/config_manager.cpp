#include "config_manager.hpp"
#include "flash_storage.hpp"

#include <cstring>

namespace config {

using namespace zdc::protocol;

const PersistentConfig ConfigManager::default_config_ = {
    .magic = FlashConfigMagic,
    .version = FlashConfigVersion,
    .display_cfg = {
        .mode = DisplayMode::HwMonitor,
        .orientation = 0,
        .brightness = 80
    },
    .keys = {
        {ReportType::Consumer, 0x00E9}, // Volume Increment
        {ReportType::Consumer, 0x00EA}, // Volume Decrement
        {ReportType::Consumer, 0x00CD}, // Play / Pause
        {ReportType::Consumer, 0x00B5}, // Next Track
        {ReportType::Consumer, 0x00B6}, // Previous Track
        {ReportType::Keyboard, 0x28},   // Enter
        {ReportType::Keyboard, 0x29},   // Escape
        {ReportType::Keyboard, 0x2B},   // Tab
        {ReportType::Keyboard, 0x2C},   // Space
        {ReportType::Keyboard, 0x06},   // 'C'
        {ReportType::Keyboard, 0x19},   // 'V'
        {ReportType::Keyboard, 0x1D},   // 'Z'
        {ReportType::Keyboard, 0x68},   // F13
        {ReportType::Keyboard, 0x69},   // F14
        {ReportType::Keyboard, 0x6A},   // F15
        {ReportType::Keyboard, 0x6B},   // F16
    },
    .checksum = 0
};

void ConfigManager::init() {
    auto& storage = zdc::storage::FlashStorage::instance();
    storage.init();

    auto stored = storage.read();
    if (stored.has_value()) {
        display_cfg_ = stored->display_cfg;
        for (size_t i = 0; i < NumMacroKeys; ++i) {
            keys_[i].config = stored->keys[i];
            keys_[i].was_pressed = false;
        }
    } else {
        display_cfg_ = default_config_.display_cfg;
        for (size_t i = 0; i < NumMacroKeys; ++i) {
            keys_[i].config = default_config_.keys[i];
            keys_[i].was_pressed = false;
        }
        save_to_flash();
    }
}

bool ConfigManager::save_to_flash() {
    PersistentConfig to_save{};
    to_save.display_cfg = display_cfg_;
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        to_save.keys[i] = keys_[i].config;
    }
    return zdc::storage::FlashStorage::instance().write(to_save);
}

void ConfigManager::handle_packet(std::span<const uint8_t> buffer) {
    if (buffer.empty()) return;

    const uint8_t cmd_byte = buffer[0];

    if (cmd_byte == static_cast<uint8_t>(Command::SaveToFlash)) {
        save_to_flash();
        return;
    }

    if (buffer.size() < 4) return;

    if (cmd_byte == static_cast<uint8_t>(Command::ConfigTargetDisplay)) {
        display_cfg_.mode = static_cast<DisplayMode>(buffer[1]);
        display_cfg_.orientation = buffer[2];
        display_cfg_.brightness = buffer[3];

        if (on_brightness_changed_) {
            on_brightness_changed_(display_cfg_.brightness);
        }
        if (on_display_changed_) {
            on_display_changed_(display_cfg_);
        }
        return;
    }

    if (cmd_byte < NumMacroKeys) {
        keys_[cmd_byte].config.report_type = static_cast<ReportType>(buffer[1]);
        keys_[cmd_byte].config.usage_code  = static_cast<uint16_t>(buffer[2] | (buffer[3] << 8));
    }
}

} // namespace config
#include "config_manager.hpp"
#include "flash_storage.hpp"

#include <array>
#include <cstring>

namespace config {

using namespace zdc::protocol;

const PersistentConfig ConfigManager::default_config_ = {
    .magic = FlashConfigMagic,
    .version = FlashConfigVersion,
    .display_cfg = {.mode = DisplayMode::HwMonitor, .orientation = 0, .brightness = 80},
    .keys =
        {
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
    .checksum = 0};

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
    if (buffer.empty())
        return;

    const uint8_t cmd_byte = buffer[0];

    if (cmd_byte == static_cast<uint8_t>(Command::SaveToFlash)) {
        save_to_flash();
        return;
    }

    if (buffer.size() < 4)
        return;

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
        keys_[cmd_byte].config.usage_code = static_cast<uint16_t>(buffer[2] | (buffer[3] << 8));
    }
}

namespace {

constexpr std::array<KeyConfig, NumMacroKeys> NumpadPreset = {{
    {ReportType::Keyboard, 0x5F}, // KP 7
    {ReportType::Keyboard, 0x60}, // KP 8
    {ReportType::Keyboard, 0x61}, // KP 9
    {ReportType::Keyboard, 0x54}, // KP /
    {ReportType::Keyboard, 0x5C}, // KP 4
    {ReportType::Keyboard, 0x5D}, // KP 5
    {ReportType::Keyboard, 0x5E}, // KP 6
    {ReportType::Keyboard, 0x55}, // KP *
    {ReportType::Keyboard, 0x59}, // KP 1
    {ReportType::Keyboard, 0x5A}, // KP 2
    {ReportType::Keyboard, 0x5B}, // KP 3
    {ReportType::Keyboard, 0x56}, // KP -
    {ReportType::Keyboard, 0x62}, // KP 0
    {ReportType::Keyboard, 0x63}, // KP .
    {ReportType::Keyboard, 0x58}, // KP Enter
    {ReportType::Keyboard, 0x57}, // KP +
}};

constexpr std::array<KeyConfig, NumMacroKeys> MediaPreset = {{
    {ReportType::Consumer, 0x00E2}, // Mute
    {ReportType::Consumer, 0x00EA}, // Vol Down
    {ReportType::Consumer, 0x00E9}, // Vol Up
    {ReportType::Consumer, 0x00CD}, // Play / Pause
    {ReportType::Consumer, 0x00B6}, // Prev Track
    {ReportType::Consumer, 0x00B5}, // Next Track
    {ReportType::Consumer, 0x00B7}, // Stop
    {ReportType::Consumer, 0x00B3}, // Fast Forward
    {ReportType::Consumer, 0x00B4}, // Rewind
    {ReportType::Consumer, 0x0192}, // Calculator
    {ReportType::Consumer, 0x0194}, // File Browser
    {ReportType::Consumer, 0x0223}, // Home
    {ReportType::Consumer, 0x0070}, // Brightness Down
    {ReportType::Consumer, 0x006F}, // Brightness Up
    {ReportType::Consumer, 0x0032}, // Sleep
    {ReportType::Consumer, 0x00B8}, // Eject
}};

constexpr std::array<KeyConfig, NumMacroKeys> NavPreset = {{
    {ReportType::Keyboard, 0x29}, // Escape
    {ReportType::Keyboard, 0x4A}, // Home
    {ReportType::Keyboard, 0x52}, // Up Arrow
    {ReportType::Keyboard, 0x4B}, // Page Up
    {ReportType::Keyboard, 0x2B}, // Tab
    {ReportType::Keyboard, 0x50}, // Left Arrow
    {ReportType::Keyboard, 0x51}, // Down Arrow
    {ReportType::Keyboard, 0x4F}, // Right Arrow
    {ReportType::Keyboard, 0x2A}, // Backspace
    {ReportType::Keyboard, 0x4D}, // End
    {ReportType::Keyboard, 0x28}, // Enter
    {ReportType::Keyboard, 0x4E}, // Page Down
    {ReportType::Keyboard, 0x68}, // F13
    {ReportType::Keyboard, 0x69}, // F14
    {ReportType::Keyboard, 0x6A}, // F15
    {ReportType::Keyboard, 0x4C}, // Delete
}};

} // namespace

std::string_view ConfigManager::active_profile_name() const {
    switch (current_profile_) {
    case 0:
        return "Custom";
    case 1:
        return "Numpad";
    case 2:
        return "Media";
    case 3:
        return "Navigation";
    default:
        return "Custom";
    }
}

void ConfigManager::next_profile() {
    current_profile_ = (current_profile_ + 1) % 4;
}

void ConfigManager::prev_profile() {
    current_profile_ = (current_profile_ == 0) ? 3 : (current_profile_ - 1);
}

const KeyConfig& ConfigManager::get_active_key_config(uint8_t index) const {
    if (index >= NumMacroKeys) {
        static const KeyConfig fallback{};
        return fallback;
    }

    switch (current_profile_) {
    case 1:
        return NumpadPreset[index];
    case 2:
        return MediaPreset[index];
    case 3:
        return NavPreset[index];
    default:
        return keys_[index].config;
    }
}

bool ConfigManager::reset_to_factory_defaults() {
    display_cfg_ = default_config_.display_cfg;
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        keys_[i].config = default_config_.keys[i];
        keys_[i].was_pressed = false;
    }
    return zdc::storage::FlashStorage::instance().write(default_config_);
}

} // namespace config
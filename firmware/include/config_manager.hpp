#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#include "protocol.hpp"

namespace config {

class ConfigManager {
public:
    using BrightnessCallback    = std::function<void(uint8_t brightness)>;
    using DisplayConfigCallback = std::function<void(const zdc::protocol::DisplayConfig& cfg)>;

    static ConfigManager& instance() {
        static ConfigManager inst;
        return inst;
    }

    // Initialize flash, load stored settings or defaults
    void init();

    // Callbacks for hardware reactive updates
    void set_on_brightness_changed(BrightnessCallback cb) { on_brightness_changed_ = std::move(cb); }
    void set_on_display_changed(DisplayConfigCallback cb) { on_display_changed_ = std::move(cb); }

    // Packet processing from USB HID / serial host
    void handle_packet(std::span<const uint8_t> buffer);

    // Getters
    [[nodiscard]] const zdc::protocol::DisplayConfig& display_config() const { return display_cfg_; }
    [[nodiscard]] zdc::protocol::DisplayConfig& display_config() { return display_cfg_; }

    [[nodiscard]] std::span<zdc::protocol::MacroKey> keys() {
        return {keys_, zdc::protocol::NumMacroKeys};
    }
    [[nodiscard]] std::span<const zdc::protocol::MacroKey> keys() const {
        return {keys_, zdc::protocol::NumMacroKeys};
    }

    [[nodiscard]] zdc::protocol::MacroKey* keys_data() { return keys_; }

    // Commit current configuration to flash
    bool save_to_flash();

private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    zdc::protocol::DisplayConfig display_cfg_{};
    zdc::protocol::MacroKey keys_[zdc::protocol::NumMacroKeys]{};

    BrightnessCallback on_brightness_changed_{nullptr};
    DisplayConfigCallback on_display_changed_{nullptr};

    static const zdc::protocol::PersistentConfig default_config_;
};

} // namespace config
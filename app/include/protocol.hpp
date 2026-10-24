#pragma once

#include <cstddef>
#include <cstdint>

namespace zdc::protocol {

// ---------------------------------------------------------------------------
// Constants & Magic Numbers
// ---------------------------------------------------------------------------
inline constexpr size_t ConfigReportSize = 64;
inline constexpr size_t NumMacroKeys = 16;
inline constexpr uint32_t FlashConfigMagic = 0x5A444331; // ASCII "ZDC1"
inline constexpr uint8_t FlashConfigVersion = 2;

// ---------------------------------------------------------------------------
// HID Report IDs
// ---------------------------------------------------------------------------
enum class ReportId : uint8_t {
    Keyboard = 1,
    ConsumerControl = 2,
    ConfigFeature = 3,
};

// ---------------------------------------------------------------------------
// Protocol Commands (Byte 0 in Feature / Config Packets)
// ---------------------------------------------------------------------------
enum class Command : uint8_t {
    Bootloader = 0xBB,          // Reboots board to BOOTSEL / UF2 drive
    SaveToFlash = 0xAA,         // Persists current RAM config to flash
    ConfigTargetDisplay = 0xFF, // Configures screen mode, rotation, brightness
};

// ---------------------------------------------------------------------------
// Report Types (Keyboard vs Media)
// ---------------------------------------------------------------------------
enum class ReportType : uint8_t {
    Keyboard = 0x01,
    Consumer = 0x02,
};

// ---------------------------------------------------------------------------
// Binding Types (Normal vs TapDance)
// ---------------------------------------------------------------------------
enum class BindingType : uint8_t {
    Normal = 0x00,   // Key down sends immediate HID report (0 ms latency)
    TapDance = 0x01, // Key events intercepted by tap-dance engine & buffer
};

// ---------------------------------------------------------------------------
// Display Modes
// ---------------------------------------------------------------------------
enum class DisplayMode : uint8_t {
    HwMonitor = 0,
    Profile = 1,
    Image = 2,
};

// ---------------------------------------------------------------------------
// Data Structures
// ---------------------------------------------------------------------------

// Display configuration payload
struct [[gnu::packed]] DisplayConfig {
    DisplayMode mode{DisplayMode::HwMonitor};
    uint8_t orientation{0}; // 0: 0°, 1: 90°, 2: 180°, 3: 270°
    uint8_t brightness{80}; // 0 - 100%
};

// Persistent key definition (stored in Flash)
struct [[gnu::packed]] KeyConfig {
    ReportType report_type{ReportType::Keyboard};
    BindingType binding_type{BindingType::Normal};
    uint16_t usage_code{0};
};

// Runtime state wrapper (keeps volatile debounce/tracking flags out of flash)
struct MacroKey {
    KeyConfig config{};
    bool was_pressed{false};
};

// Flash persistent image (aligned to 4 bytes for RP2040 flash controller)
struct alignas(4) PersistentConfig {
    uint32_t magic{FlashConfigMagic};
    uint8_t version{FlashConfigVersion};
    DisplayConfig display_cfg{};
    KeyConfig keys[NumMacroKeys]{};
    uint8_t checksum{0};
};

} // namespace zdc::protocol

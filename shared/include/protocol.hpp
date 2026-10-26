#pragma once

#include <cstddef>
#include <cstdint>

namespace zdc::protocol {

// ---------------------------------------------------------------------------
// Constants & Magic Numbers
// ---------------------------------------------------------------------------
inline constexpr size_t ConfigReportSize = 64;
inline constexpr size_t NumMacroKeys = 16;
inline constexpr size_t MaxProfiles = 4;
inline constexpr size_t ProfileNameLength = 16;
inline constexpr uint32_t FlashConfigMagic = 0x5A444331; // ASCII "ZDC1"
inline constexpr uint8_t FlashConfigVersion = 3;

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
// ---------------------------------------------------------------------------
// Binding Types (Normal vs TapDance)
// ---------------------------------------------------------------------------
enum class BindingType : uint8_t {
    Normal = 0x00,   // Key down sends immediate HID report (0 ms latency)
    TapDance = 0x01, // Key events intercepted by tap-dance engine & buffer
};

// ---------------------------------------------------------------------------
// Standard HID Keycodes
// ---------------------------------------------------------------------------
enum class KeyCode : uint8_t {
    None = 0x00,
    Return = 0x28,
    Escape = 0x29,
    Backspace = 0x2A,
    Tab = 0x2B,
    Space = 0x2C,
    PrintScreen = 0x46,
    Home = 0x4A,
    PageUp = 0x4B,
    Delete = 0x4C,
    End = 0x4D,
    PageDown = 0x4E,
    RightArrow = 0x4F,
    LeftArrow = 0x50,
    DownArrow = 0x51,
    UpArrow = 0x52,
    F13 = 0x68,
    F14 = 0x69,
    F15 = 0x6A,
    F16 = 0x6B,
};

// ---------------------------------------------------------------------------
// Composite System Actions / Macros
// ---------------------------------------------------------------------------
enum class MacroCode : uint16_t {
    LockScreenWinLinux = 0x1001, // Win + L
    LockScreenMac = 0x1002,      // Cmd + Ctrl + Q
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

// Persistent profile definition (stored in Flash)
struct [[gnu::packed]] ProfileConfig {
    char name[ProfileNameLength]{};
    KeyConfig keys[NumMacroKeys]{};
};

// Flash persistent image (aligned to 4 bytes for RP2040 flash controller)
struct alignas(4) PersistentConfig {
    uint32_t magic{FlashConfigMagic};
    uint8_t version{FlashConfigVersion};
    DisplayConfig display_cfg{};
    uint8_t profile_count{1};
    uint8_t active_profile{0};
    uint8_t reserved[2]{0, 0};
    ProfileConfig profiles[MaxProfiles]{};
    uint8_t checksum{0};
};

} // namespace zdc::protocol

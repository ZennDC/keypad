#include <catch2/catch_test_macros.hpp>
#include <protocol.hpp>

#include <array>
#include <cstddef>
#include <cstring>

using namespace zdc::protocol;

namespace {

uint8_t calculate_checksum(const PersistentConfig& cfg) {
    uint8_t sum = 0;
    const auto* bytes = reinterpret_cast<const uint8_t*>(&cfg);
    constexpr size_t len = offsetof(PersistentConfig, checksum);

    for (size_t i = 0; i < len; ++i) {
        sum += bytes[i];
    }
    return sum;
}

} // namespace

TEST_CASE("Protocol constants and limits", "[protocol]") {
    STATIC_REQUIRE(ConfigReportSize == 64);
    STATIC_REQUIRE(NumMacroKeys == 16);
    STATIC_REQUIRE(FlashConfigMagic == 0x5A444331); // "ZDC1"
    STATIC_REQUIRE(FlashConfigVersion == 2);
}

TEST_CASE("Enum definitions match hardware protocol specification", "[protocol]") {
    STATIC_REQUIRE(static_cast<uint8_t>(ReportId::Keyboard) == 1);
    STATIC_REQUIRE(static_cast<uint8_t>(ReportId::ConsumerControl) == 2);
    STATIC_REQUIRE(static_cast<uint8_t>(ReportId::ConfigFeature) == 3);

    STATIC_REQUIRE(static_cast<uint8_t>(Command::Bootloader) == 0xBB);
    STATIC_REQUIRE(static_cast<uint8_t>(Command::SaveToFlash) == 0xAA);
    STATIC_REQUIRE(static_cast<uint8_t>(Command::ConfigTargetDisplay) == 0xFF);

    STATIC_REQUIRE(static_cast<uint8_t>(ReportType::Keyboard) == 0x01);
    STATIC_REQUIRE(static_cast<uint8_t>(ReportType::Consumer) == 0x02);

    STATIC_REQUIRE(static_cast<uint8_t>(BindingType::Normal) == 0x00);
    STATIC_REQUIRE(static_cast<uint8_t>(BindingType::TapDance) == 0x01);

    STATIC_REQUIRE(static_cast<uint8_t>(DisplayMode::HwMonitor) == 0);
    STATIC_REQUIRE(static_cast<uint8_t>(DisplayMode::Profile) == 1);
    STATIC_REQUIRE(static_cast<uint8_t>(DisplayMode::Image) == 2);

    // Standard HID Keycodes
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::None) == 0x00);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::Return) == 0x28);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::Escape) == 0x29);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::Backspace) == 0x2A);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::Tab) == 0x2B);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::Space) == 0x2C);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::PrintScreen) == 0x46);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::Home) == 0x4A);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::PageUp) == 0x4B);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::Delete) == 0x4C);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::End) == 0x4D);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::PageDown) == 0x4E);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::RightArrow) == 0x4F);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::LeftArrow) == 0x50);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::DownArrow) == 0x51);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::UpArrow) == 0x52);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::F13) == 0x68);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::F14) == 0x69);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::F15) == 0x6A);
    STATIC_REQUIRE(static_cast<uint8_t>(KeyCode::F16) == 0x6B);

    // Composite System Actions / Macros
    STATIC_REQUIRE(static_cast<uint16_t>(MacroCode::LockScreenWinLinux) == 0x1001);
    STATIC_REQUIRE(static_cast<uint16_t>(MacroCode::LockScreenMac) == 0x1002);
}

TEST_CASE("Data structures packing, sizes and memory layout", "[protocol]") {
    SECTION("DisplayConfig byte packing") {
        STATIC_REQUIRE(sizeof(DisplayConfig) == 3);
        STATIC_REQUIRE(offsetof(DisplayConfig, mode) == 0);
        STATIC_REQUIRE(offsetof(DisplayConfig, orientation) == 1);
        STATIC_REQUIRE(offsetof(DisplayConfig, brightness) == 2);
    }

    SECTION("KeyConfig byte packing") {
        STATIC_REQUIRE(sizeof(KeyConfig) == 4);
        STATIC_REQUIRE(offsetof(KeyConfig, report_type) == 0);
        STATIC_REQUIRE(offsetof(KeyConfig, binding_type) == 1);
        STATIC_REQUIRE(offsetof(KeyConfig, usage_code) == 2);
    }

    SECTION("PersistentConfig alignment and layout") {
        // RP2040 flash controller requires 4-byte word alignment
        STATIC_REQUIRE(alignof(PersistentConfig) == 4);

        // Verify member offsets to guarantee host-to-device memory layout stability
        STATIC_REQUIRE(offsetof(PersistentConfig, magic) == 0);
        STATIC_REQUIRE(offsetof(PersistentConfig, version) == 4);
        STATIC_REQUIRE(offsetof(PersistentConfig, display_cfg) == 5);
        STATIC_REQUIRE(offsetof(PersistentConfig, keys) == 8);
        STATIC_REQUIRE(offsetof(PersistentConfig, checksum) == 72);
        STATIC_REQUIRE(sizeof(PersistentConfig) == 76);
    }
}

TEST_CASE("PersistentConfig checksum calculation and validation", "[protocol]") {
    PersistentConfig config{};
    config.magic = FlashConfigMagic;
    config.version = FlashConfigVersion;
    config.display_cfg.mode = DisplayMode::Profile;
    config.display_cfg.orientation = 2;
    config.display_cfg.brightness = 90;

    for (size_t i = 0; i < NumMacroKeys; ++i) {
        config.keys[i].report_type = (i % 2 == 0) ? ReportType::Keyboard : ReportType::Consumer;
        config.keys[i].binding_type = (i % 3 == 0) ? BindingType::TapDance : BindingType::Normal;
        config.keys[i].usage_code = static_cast<uint16_t>(0x1000 + i);
    }

    config.checksum = calculate_checksum(config);

    SECTION("Checksum matches computed value on untouched struct") {
        REQUIRE(config.checksum == calculate_checksum(config));
    }

    SECTION("Modifying display settings invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.display_cfg.brightness = 40;
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }

    SECTION("Modifying a single key code invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.keys[5].usage_code = 0x9999;
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }

    SECTION("Modifying key report type invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.keys[0].report_type = ReportType::Consumer;
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }

    SECTION("Modifying key binding type invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.keys[1].binding_type = BindingType::TapDance;
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }
}

TEST_CASE("Flash persistence serialization round-trip", "[protocol]") {
    PersistentConfig original{};
    original.magic = FlashConfigMagic;
    original.version = FlashConfigVersion;
    original.display_cfg.mode = DisplayMode::HwMonitor;
    original.display_cfg.orientation = 1;
    original.display_cfg.brightness = 75;

    for (size_t i = 0; i < NumMacroKeys; ++i) {
        original.keys[i].report_type = ReportType::Keyboard;
        original.keys[i].binding_type = (i % 2 == 0) ? BindingType::Normal : BindingType::TapDance;
        original.keys[i].usage_code = static_cast<uint16_t>(0x04 + i); // 'A', 'B', ...
    }
    original.checksum = calculate_checksum(original);

    // Simulate writing to and reading from flash storage buffer
    std::array<uint8_t, sizeof(PersistentConfig)> storage_buffer{};
    std::memcpy(storage_buffer.data(), &original, sizeof(PersistentConfig));

    PersistentConfig reconstructed{};
    std::memcpy(&reconstructed, storage_buffer.data(), sizeof(PersistentConfig));

    REQUIRE(reconstructed.magic == original.magic);
    REQUIRE(reconstructed.version == original.version);
    REQUIRE(reconstructed.display_cfg.mode == original.display_cfg.mode);
    REQUIRE(reconstructed.display_cfg.orientation == original.display_cfg.orientation);
    REQUIRE(reconstructed.display_cfg.brightness == original.display_cfg.brightness);
    REQUIRE(reconstructed.checksum == original.checksum);
    REQUIRE(reconstructed.checksum == calculate_checksum(reconstructed));

    for (size_t i = 0; i < NumMacroKeys; ++i) {
        REQUIRE(reconstructed.keys[i].report_type == original.keys[i].report_type);
        REQUIRE(reconstructed.keys[i].binding_type == original.keys[i].binding_type);
        REQUIRE(reconstructed.keys[i].usage_code == original.keys[i].usage_code);
    }
}

TEST_CASE("Legacy V1 layout upgrade to V2 with default NORMAL binding", "[protocol]") {
    struct [[gnu::packed]] KeyConfigV1 {
        ReportType report_type{ReportType::Keyboard};
        uint16_t usage_code{0};
    };

    struct alignas(4) PersistentConfigV1 {
        uint32_t magic{FlashConfigMagic};
        uint8_t version{1};
        DisplayConfig display_cfg{};
        KeyConfigV1 keys[NumMacroKeys]{};
        uint8_t checksum{0};
    };

    PersistentConfigV1 v1{};
    v1.magic = FlashConfigMagic;
    v1.version = 1;
    v1.display_cfg.mode = DisplayMode::Profile;
    v1.display_cfg.orientation = 0;
    v1.display_cfg.brightness = 80;

    for (size_t i = 0; i < NumMacroKeys; ++i) {
        v1.keys[i].report_type = ReportType::Keyboard;
        v1.keys[i].usage_code = static_cast<uint16_t>(0x28 + i);
    }

    uint8_t sum = 0;
    const auto* bytes = reinterpret_cast<const uint8_t*>(&v1);
    constexpr size_t len = offsetof(PersistentConfigV1, checksum);
    for (size_t i = 0; i < len; ++i) {
        sum += bytes[i];
    }
    v1.checksum = sum;

    // Simulate FlashStorage migration logic
    PersistentConfig upgraded{};
    upgraded.magic = FlashConfigMagic;
    upgraded.version = FlashConfigVersion;
    upgraded.display_cfg = v1.display_cfg;
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        upgraded.keys[i].report_type = v1.keys[i].report_type;
        upgraded.keys[i].binding_type = BindingType::Normal;
        upgraded.keys[i].usage_code = v1.keys[i].usage_code;
    }
    upgraded.checksum = calculate_checksum(upgraded);

    REQUIRE(upgraded.version == 2);
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        REQUIRE(upgraded.keys[i].binding_type == BindingType::Normal);
        REQUIRE(upgraded.keys[i].report_type == v1.keys[i].report_type);
        REQUIRE(upgraded.keys[i].usage_code == v1.keys[i].usage_code);
    }
}

TEST_CASE("USB HID LED Indicator bit flag extraction", "[usb][led]") {
    // HID Keyboard Output report defines:
    // Bit 0: Num Lock (0x01)
    // Bit 1: Caps Lock (0x02)
    // Bit 2: Scroll Lock (0x04)
    constexpr uint8_t NumLockFlag = 1 << 0;
    constexpr uint8_t CapsLockFlag = 1 << 1;
    constexpr uint8_t ScrollLockFlag = 1 << 2;

    SECTION("Bit flag positions match USB HID LED specification") {
        STATIC_REQUIRE(NumLockFlag == 0x01);
        STATIC_REQUIRE(CapsLockFlag == 0x02);
        STATIC_REQUIRE(ScrollLockFlag == 0x04);
    }

    SECTION("Extract flags from simulated host 1-byte output report") {
        uint8_t report_byte = 0;
        REQUIRE((report_byte & NumLockFlag) == 0);
        REQUIRE((report_byte & CapsLockFlag) == 0);
        REQUIRE((report_byte & ScrollLockFlag) == 0);

        // Host turns on Caps Lock (0x02)
        report_byte = 0x02;
        REQUIRE((report_byte & NumLockFlag) == 0);
        REQUIRE((report_byte & CapsLockFlag) != 0);
        REQUIRE((report_byte & ScrollLockFlag) == 0);

        // Host turns on Num Lock + Caps Lock (0x03)
        report_byte = 0x03;
        REQUIRE((report_byte & NumLockFlag) != 0);
        REQUIRE((report_byte & CapsLockFlag) != 0);
        REQUIRE((report_byte & ScrollLockFlag) == 0);

        // Host turns on Num Lock + Scroll Lock (0x05)
        report_byte = 0x05;
        REQUIRE((report_byte & NumLockFlag) != 0);
        REQUIRE((report_byte & CapsLockFlag) == 0);
        REQUIRE((report_byte & ScrollLockFlag) != 0);

        // Host turns on all three (0x07)
        report_byte = 0x07;
        REQUIRE((report_byte & NumLockFlag) != 0);
        REQUIRE((report_byte & CapsLockFlag) != 0);
        REQUIRE((report_byte & ScrollLockFlag) != 0);
    }
}

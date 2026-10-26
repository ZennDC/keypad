#include <catch2/catch_test_macros.hpp>
#include <flash_storage.hpp>
#include <protocol.hpp>

#include <array>
#include <cstddef>
#include <cstring>

using namespace zdc::protocol;
using namespace zdc::storage;

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
    STATIC_REQUIRE(MaxProfiles == 4);
    STATIC_REQUIRE(ProfileNameLength == 16);
    STATIC_REQUIRE(FlashConfigMagic == 0x5A444331); // "ZDC1"
    STATIC_REQUIRE(FlashConfigVersion == 3);
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

    SECTION("ProfileConfig byte packing") {
        STATIC_REQUIRE(sizeof(ProfileConfig) == 80);
        STATIC_REQUIRE(offsetof(ProfileConfig, name) == 0);
        STATIC_REQUIRE(offsetof(ProfileConfig, keys) == 16);
    }

    SECTION("PersistentConfig alignment and layout") {
        // RP2040 flash controller requires 4-byte word alignment
        STATIC_REQUIRE(alignof(PersistentConfig) == 4);

        // Verify member offsets to guarantee host-to-device memory layout stability
        STATIC_REQUIRE(offsetof(PersistentConfig, magic) == 0);
        STATIC_REQUIRE(offsetof(PersistentConfig, version) == 4);
        STATIC_REQUIRE(offsetof(PersistentConfig, display_cfg) == 5);
        STATIC_REQUIRE(offsetof(PersistentConfig, profile_count) == 8);
        STATIC_REQUIRE(offsetof(PersistentConfig, active_profile) == 9);
        STATIC_REQUIRE(offsetof(PersistentConfig, reserved) == 10);
        STATIC_REQUIRE(offsetof(PersistentConfig, profiles) == 12);
        STATIC_REQUIRE(offsetof(PersistentConfig, checksum) == 332);
        STATIC_REQUIRE(sizeof(PersistentConfig) == 336);
    }
}

TEST_CASE("PersistentConfig checksum calculation and validation", "[protocol]") {
    PersistentConfig config{};
    config.magic = FlashConfigMagic;
    config.version = FlashConfigVersion;
    config.display_cfg.mode = DisplayMode::Profile;
    config.display_cfg.orientation = 2;
    config.display_cfg.brightness = 90;
    config.profile_count = 2;
    config.active_profile = 0;
    config.reserved[0] = 0;
    config.reserved[1] = 0;

    std::strncpy(config.profiles[0].name, "Default", ProfileNameLength - 1);
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        config.profiles[0].keys[i].report_type = (i % 2 == 0) ? ReportType::Keyboard : ReportType::Consumer;
        config.profiles[0].keys[i].binding_type = (i % 3 == 0) ? BindingType::TapDance : BindingType::Normal;
        config.profiles[0].keys[i].usage_code = static_cast<uint16_t>(0x1000 + i);
    }

    std::strncpy(config.profiles[1].name, "Media", ProfileNameLength - 1);
    for (size_t i = 0; i < NumMacroKeys; ++i) {
        config.profiles[1].keys[i].report_type = ReportType::Consumer;
        config.profiles[1].keys[i].binding_type = BindingType::Normal;
        config.profiles[1].keys[i].usage_code = static_cast<uint16_t>(0x00E0 + i);
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

    SECTION("Modifying profile_count invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.profile_count = 3;
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }

    SECTION("Modifying active_profile invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.active_profile = 1;
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }

    SECTION("Modifying a single key code in profile 0 invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.profiles[0].keys[5].usage_code = 0x9999;
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }

    SECTION("Modifying a single key code in profile 1 invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.profiles[1].keys[2].usage_code = 0x8888;
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }

    SECTION("Modifying profile name invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.profiles[0].name[0] = 'X';
        REQUIRE(corrupted.checksum != calculate_checksum(corrupted));
    }

    SECTION("Modifying key binding type invalidates checksum") {
        PersistentConfig corrupted = config;
        corrupted.profiles[0].keys[1].binding_type = BindingType::TapDance;
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
    original.profile_count = 3;
    original.active_profile = 1;

    for (size_t p = 0; p < original.profile_count; ++p) {
        std::snprintf(original.profiles[p].name, ProfileNameLength, "Prof_%zu", p);
        for (size_t i = 0; i < NumMacroKeys; ++i) {
            original.profiles[p].keys[i].report_type = ReportType::Keyboard;
            original.profiles[p].keys[i].binding_type = (i % 2 == 0) ? BindingType::Normal : BindingType::TapDance;
            original.profiles[p].keys[i].usage_code = static_cast<uint16_t>(0x04 + (p * 16) + i);
        }
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
    REQUIRE(reconstructed.profile_count == original.profile_count);
    REQUIRE(reconstructed.active_profile == original.active_profile);
    REQUIRE(reconstructed.checksum == original.checksum);
    REQUIRE(reconstructed.checksum == calculate_checksum(reconstructed));

    for (size_t p = 0; p < original.profile_count; ++p) {
        REQUIRE(std::strncmp(reconstructed.profiles[p].name, original.profiles[p].name, ProfileNameLength) == 0);
        for (size_t i = 0; i < NumMacroKeys; ++i) {
            REQUIRE(reconstructed.profiles[p].keys[i].report_type == original.profiles[p].keys[i].report_type);
            REQUIRE(reconstructed.profiles[p].keys[i].binding_type == original.profiles[p].keys[i].binding_type);
            REQUIRE(reconstructed.profiles[p].keys[i].usage_code == original.profiles[p].keys[i].usage_code);
        }
    }
}

TEST_CASE("Legacy V1 and V2 layout upgrade to V3", "[protocol]") {
    SECTION("Legacy V1 layout upgrade to V3 with default NORMAL binding") {
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

        // Inject raw V1 flash image into FlashStorage
        FlashStorage::test_set_flash_raw(&v1, sizeof(v1));

        auto migrated = FlashStorage::instance().read();
        REQUIRE(migrated.has_value());
        REQUIRE(migrated->version == 3);
        REQUIRE(migrated->profile_count == 1);
        REQUIRE(migrated->active_profile == 0);
        REQUIRE(std::strcmp(migrated->profiles[0].name, "Default") == 0);
        REQUIRE(migrated->display_cfg.mode == v1.display_cfg.mode);
        REQUIRE(migrated->display_cfg.brightness == v1.display_cfg.brightness);

        for (size_t i = 0; i < NumMacroKeys; ++i) {
            REQUIRE(migrated->profiles[0].keys[i].binding_type == BindingType::Normal);
            REQUIRE(migrated->profiles[0].keys[i].report_type == v1.keys[i].report_type);
            REQUIRE(migrated->profiles[0].keys[i].usage_code == v1.keys[i].usage_code);
        }
        REQUIRE(migrated->checksum == calculate_checksum(*migrated));
    }

    SECTION("Legacy V2 layout upgrade to V3 preserving binding_type") {
        struct alignas(4) PersistentConfigV2 {
            uint32_t magic{FlashConfigMagic};
            uint8_t version{2};
            DisplayConfig display_cfg{};
            KeyConfig keys[NumMacroKeys]{};
            uint8_t checksum{0};
        };

        PersistentConfigV2 v2{};
        v2.magic = FlashConfigMagic;
        v2.version = 2;
        v2.display_cfg.mode = DisplayMode::HwMonitor;
        v2.display_cfg.orientation = 1;
        v2.display_cfg.brightness = 70;

        for (size_t i = 0; i < NumMacroKeys; ++i) {
            v2.keys[i].report_type = (i % 2 == 0) ? ReportType::Keyboard : ReportType::Consumer;
            v2.keys[i].binding_type = (i % 3 == 0) ? BindingType::TapDance : BindingType::Normal;
            v2.keys[i].usage_code = static_cast<uint16_t>(0x0050 + i);
        }

        uint8_t sum = 0;
        const auto* bytes = reinterpret_cast<const uint8_t*>(&v2);
        constexpr size_t len = offsetof(PersistentConfigV2, checksum);
        for (size_t i = 0; i < len; ++i) {
            sum += bytes[i];
        }
        v2.checksum = sum;

        // Inject raw V2 flash image into FlashStorage
        FlashStorage::test_set_flash_raw(&v2, sizeof(v2));

        auto migrated = FlashStorage::instance().read();
        REQUIRE(migrated.has_value());
        REQUIRE(migrated->version == 3);
        REQUIRE(migrated->profile_count == 1);
        REQUIRE(migrated->active_profile == 0);
        REQUIRE(std::strcmp(migrated->profiles[0].name, "Default") == 0);
        REQUIRE(migrated->display_cfg.mode == v2.display_cfg.mode);
        REQUIRE(migrated->display_cfg.orientation == v2.display_cfg.orientation);
        REQUIRE(migrated->display_cfg.brightness == v2.display_cfg.brightness);

        for (size_t i = 0; i < NumMacroKeys; ++i) {
            REQUIRE(migrated->profiles[0].keys[i].binding_type == v2.keys[i].binding_type);
            REQUIRE(migrated->profiles[0].keys[i].report_type == v2.keys[i].report_type);
            REQUIRE(migrated->profiles[0].keys[i].usage_code == v2.keys[i].usage_code);
        }
        REQUIRE(migrated->checksum == calculate_checksum(*migrated));
    }
}

TEST_CASE("FlashStorage variable profile management and bounds checking", "[storage][profile]") {
    FlashStorage::test_reset_flash();
    auto& storage = FlashStorage::instance();

    SECTION("Init on blank flash creates defaults with 1 profile") {
        storage.init();
        REQUIRE(storage.profile_count() == 1);
        REQUIRE(storage.active_profile_index() == 0);

        auto p0 = storage.get_profile(0);
        REQUIRE(p0.has_value());
        REQUIRE(std::strcmp(p0->name, "Default") == 0);
    }

    SECTION("Adding profiles up to MaxProfiles limit") {
        storage.reset_defaults();
        REQUIRE(storage.profile_count() == 1);

        ProfileConfig p1{};
        std::strncpy(p1.name, "Numpad", ProfileNameLength - 1);
        p1.keys[0] = {ReportType::Keyboard, BindingType::Normal, 0x59};

        ProfileConfig p2{};
        std::strncpy(p2.name, "Media", ProfileNameLength - 1);
        p2.keys[0] = {ReportType::Consumer, BindingType::Normal, 0x00E2};

        ProfileConfig p3{};
        std::strncpy(p3.name, "Gaming", ProfileNameLength - 1);
        p3.keys[0] = {ReportType::Keyboard, BindingType::TapDance, 0x04};

        REQUIRE(storage.add_profile(p1) == true);
        REQUIRE(storage.profile_count() == 2);

        REQUIRE(storage.add_profile(p2) == true);
        REQUIRE(storage.profile_count() == 3);

        REQUIRE(storage.add_profile(p3) == true);
        REQUIRE(storage.profile_count() == 4);

        // Attempting to exceed MaxProfiles (4) must fail
        ProfileConfig p4{};
        std::strncpy(p4.name, "Overflow", ProfileNameLength - 1);
        REQUIRE(storage.add_profile(p4) == false);
        REQUIRE(storage.profile_count() == 4);

        // Verify all 4 profiles stored correctly
        REQUIRE(std::strcmp(storage.get_profile(0)->name, "Default") == 0);
        REQUIRE(std::strcmp(storage.get_profile(1)->name, "Numpad") == 0);
        REQUIRE(std::strcmp(storage.get_profile(2)->name, "Media") == 0);
        REQUIRE(std::strcmp(storage.get_profile(3)->name, "Gaming") == 0);
        REQUIRE(storage.get_profile(4).has_value() == false);
    }

    SECTION("Active profile switching and bounds checking") {
        storage.reset_defaults();
        ProfileConfig p1{};
        std::strncpy(p1.name, "Profile2", ProfileNameLength - 1);
        storage.add_profile(p1);

        REQUIRE(storage.active_profile_index() == 0);
        REQUIRE(storage.set_active_profile_index(1) == true);
        REQUIRE(storage.active_profile_index() == 1);

        // Out of bounds profile index selection must fail
        REQUIRE(storage.set_active_profile_index(2) == false);
        REQUIRE(storage.active_profile_index() == 1);
    }

    SECTION("Updating profile configuration") {
        storage.reset_defaults();
        ProfileConfig updated{};
        std::strncpy(updated.name, "CustomV2", ProfileNameLength - 1);
        updated.keys[3] = {ReportType::Keyboard, BindingType::TapDance, 0x2A};

        REQUIRE(storage.set_profile(0, updated) == true);
        auto p0 = storage.get_profile(0);
        REQUIRE(p0.has_value());
        REQUIRE(std::strcmp(p0->name, "CustomV2") == 0);
        REQUIRE(p0->keys[3].usage_code == 0x2A);
        REQUIRE(p0->keys[3].binding_type == BindingType::TapDance);

        // Out of bounds set must fail
        REQUIRE(storage.set_profile(1, updated) == false);
    }

    SECTION("Removing profiles and active index adjustment") {
        storage.reset_defaults();
        ProfileConfig p1{};
        std::strncpy(p1.name, "P1", ProfileNameLength - 1);
        ProfileConfig p2{};
        std::strncpy(p2.name, "P2", ProfileNameLength - 1);
        storage.add_profile(p1);
        storage.add_profile(p2);
        REQUIRE(storage.profile_count() == 3);

        // Set active profile to the last one (index 2)
        storage.set_active_profile_index(2);
        REQUIRE(storage.active_profile_index() == 2);

        // Remove profile at index 1 ("P1")
        REQUIRE(storage.remove_profile(1) == true);
        REQUIRE(storage.profile_count() == 2);
        // "P2" should now be shifted to index 1
        REQUIRE(std::strcmp(storage.get_profile(1)->name, "P2") == 0);
        // Active index should clamp/adjust to 1
        REQUIRE(storage.active_profile_index() == 1);

        // Remove profile at index 1 ("P2")
        REQUIRE(storage.remove_profile(1) == true);
        REQUIRE(storage.profile_count() == 1);
        REQUIRE(storage.active_profile_index() == 0);

        // Cannot remove the last remaining profile (must always keep at least 1)
        REQUIRE(storage.remove_profile(0) == false);
        REQUIRE(storage.profile_count() == 1);
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

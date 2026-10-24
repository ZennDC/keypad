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
    STATIC_REQUIRE(FlashConfigVersion == 1);
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

    STATIC_REQUIRE(static_cast<uint8_t>(DisplayMode::HwMonitor) == 0);
    STATIC_REQUIRE(static_cast<uint8_t>(DisplayMode::Profile) == 1);
    STATIC_REQUIRE(static_cast<uint8_t>(DisplayMode::Image) == 2);
}

TEST_CASE("Data structures packing, sizes and memory layout", "[protocol]") {
    SECTION("DisplayConfig byte packing") {
        STATIC_REQUIRE(sizeof(DisplayConfig) == 3);
        STATIC_REQUIRE(offsetof(DisplayConfig, mode) == 0);
        STATIC_REQUIRE(offsetof(DisplayConfig, orientation) == 1);
        STATIC_REQUIRE(offsetof(DisplayConfig, brightness) == 2);
    }

    SECTION("KeyConfig byte packing") {
        STATIC_REQUIRE(sizeof(KeyConfig) == 3);
        STATIC_REQUIRE(offsetof(KeyConfig, report_type) == 0);
        STATIC_REQUIRE(offsetof(KeyConfig, usage_code) == 1);
    }

    SECTION("PersistentConfig alignment and USB feature report budget") {
        // RP2040 flash controller requires 4-byte word alignment
        STATIC_REQUIRE(alignof(PersistentConfig) == 4);

        // PersistentConfig MUST fit inside the 64-byte USB Feature Report
        STATIC_REQUIRE(sizeof(PersistentConfig) <= ConfigReportSize);

        // Verify member offsets to guarantee host-to-device memory layout stability
        STATIC_REQUIRE(offsetof(PersistentConfig, magic) == 0);
        STATIC_REQUIRE(offsetof(PersistentConfig, version) == 4);
        STATIC_REQUIRE(offsetof(PersistentConfig, display_cfg) == 5);
        STATIC_REQUIRE(offsetof(PersistentConfig, keys) == 8);
        STATIC_REQUIRE(offsetof(PersistentConfig, checksum) == 56);
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
}

TEST_CASE("Feature report serialization round-trip", "[protocol]") {
    PersistentConfig original{};
    original.magic = FlashConfigMagic;
    original.version = FlashConfigVersion;
    original.display_cfg.mode = DisplayMode::HwMonitor;
    original.display_cfg.orientation = 1;
    original.display_cfg.brightness = 75;

    for (size_t i = 0; i < NumMacroKeys; ++i) {
        original.keys[i].report_type = ReportType::Keyboard;
        original.keys[i].usage_code = static_cast<uint16_t>(0x04 + i); // 'A', 'B', ...
    }
    original.checksum = calculate_checksum(original);

    // Simulate sending over a 64-byte USB Feature Report
    std::array<uint8_t, ConfigReportSize> report_buffer{};
    std::memcpy(report_buffer.data(), &original, sizeof(PersistentConfig));

    // Deserialization on host or receiver
    PersistentConfig reconstructed{};
    std::memcpy(&reconstructed, report_buffer.data(), sizeof(PersistentConfig));

    REQUIRE(reconstructed.magic == original.magic);
    REQUIRE(reconstructed.version == original.version);
    REQUIRE(reconstructed.display_cfg.mode == original.display_cfg.mode);
    REQUIRE(reconstructed.display_cfg.orientation == original.display_cfg.orientation);
    REQUIRE(reconstructed.display_cfg.brightness == original.display_cfg.brightness);
    REQUIRE(reconstructed.checksum == original.checksum);
    REQUIRE(reconstructed.checksum == calculate_checksum(reconstructed));

    for (size_t i = 0; i < NumMacroKeys; ++i) {
        REQUIRE(reconstructed.keys[i].report_type == original.keys[i].report_type);
        REQUIRE(reconstructed.keys[i].usage_code == original.keys[i].usage_code);
    }
}

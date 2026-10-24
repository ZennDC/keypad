#include "flash_storage.hpp"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <cstddef>
#include <cstring>

namespace zdc::storage {

namespace {
// Reserve the very last 4096-byte sector of onboard flash
constexpr uint32_t TargetOffset = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

// V1 legacy struct for backwards compatibility migration
struct [[gnu::packed]] KeyConfigV1 {
    protocol::ReportType report_type{protocol::ReportType::Keyboard};
    uint16_t usage_code{0};
};

struct alignas(4) PersistentConfigV1 {
    uint32_t magic{protocol::FlashConfigMagic};
    uint8_t version{1};
    protocol::DisplayConfig display_cfg{};
    KeyConfigV1 keys[protocol::NumMacroKeys]{};
    uint8_t checksum{0};
};

uint8_t calculate_checksum_v1(const PersistentConfigV1& cfg) {
    uint8_t sum = 0;
    const auto* bytes = reinterpret_cast<const uint8_t*>(&cfg);
    constexpr size_t len = offsetof(PersistentConfigV1, checksum);
    for (size_t i = 0; i < len; ++i) {
        sum += bytes[i];
    }
    return sum;
}

} // namespace

const protocol::PersistentConfig* FlashStorage::flash_target() {
    return reinterpret_cast<const protocol::PersistentConfig*>(XIP_BASE + TargetOffset);
}

uint8_t FlashStorage::calculate_checksum(const protocol::PersistentConfig& cfg) {
    uint8_t sum = 0;
    const auto* bytes = reinterpret_cast<const uint8_t*>(&cfg);
    constexpr size_t len = offsetof(protocol::PersistentConfig, checksum);

    for (size_t i = 0; i < len; ++i) {
        sum += bytes[i];
    }
    return sum;
}

void FlashStorage::init() {
    if (!read().has_value()) {
        reset_defaults();
    }
}

std::optional<protocol::PersistentConfig> FlashStorage::read() const {
    const auto* target = flash_target();

    if (target->magic != protocol::FlashConfigMagic) {
        return std::nullopt;
    }

    if (target->version == protocol::FlashConfigVersion) {
        if (target->checksum != calculate_checksum(*target)) {
            return std::nullopt;
        }

        protocol::PersistentConfig out{};
        std::memcpy(&out, target, sizeof(protocol::PersistentConfig));
        return out;
    }

    // Backwards compatibility: load and migrate version 1 layout
    if (target->version == 1) {
        const auto* target_v1 = reinterpret_cast<const PersistentConfigV1*>(target);
        if (target_v1->checksum != calculate_checksum_v1(*target_v1)) {
            return std::nullopt;
        }

        protocol::PersistentConfig out{};
        out.magic = protocol::FlashConfigMagic;
        out.version = protocol::FlashConfigVersion;
        out.display_cfg = target_v1->display_cfg;
        for (size_t i = 0; i < protocol::NumMacroKeys; ++i) {
            out.keys[i].report_type = target_v1->keys[i].report_type;
            out.keys[i].binding_type = protocol::BindingType::Normal;
            out.keys[i].usage_code = target_v1->keys[i].usage_code;
        }
        out.checksum = calculate_checksum(out);
        return out;
    }

    return std::nullopt;
}

bool FlashStorage::write(const protocol::PersistentConfig& config) {
    // RP2040 flash controller requires programming in minimum chunks of 256 bytes (FLASH_PAGE_SIZE)
    alignas(4) uint8_t page_buffer[FLASH_PAGE_SIZE];
    std::memset(page_buffer, 0xFF, sizeof(page_buffer));

    auto* staged_cfg = reinterpret_cast<protocol::PersistentConfig*>(page_buffer);
    std::memcpy(staged_cfg, &config, sizeof(protocol::PersistentConfig));

    staged_cfg->magic = protocol::FlashConfigMagic;
    staged_cfg->version = protocol::FlashConfigVersion;
    staged_cfg->checksum = calculate_checksum(*staged_cfg);

    // Disable interrupts before modifying XIP flash
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(TargetOffset, FLASH_SECTOR_SIZE);
    flash_range_program(TargetOffset, page_buffer, FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);

    return true;
}

bool FlashStorage::reset_defaults() {
    protocol::PersistentConfig defaults{};
    defaults.magic = protocol::FlashConfigMagic;
    defaults.version = protocol::FlashConfigVersion;
    defaults.display_cfg =
        protocol::DisplayConfig{.mode = protocol::DisplayMode::HwMonitor, .orientation = 0, .brightness = 80};

    return write(defaults);
}

} // namespace zdc::storage
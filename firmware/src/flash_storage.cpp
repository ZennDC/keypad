#include "flash_storage.hpp"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <cstring>
#include <cstddef>

namespace zdc::storage {

namespace {
// Reserve the very last 4096-byte sector of onboard flash
constexpr uint32_t TargetOffset = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
}

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

    if (target->magic != protocol::FlashConfigMagic || 
        target->version != protocol::FlashConfigVersion) {
        return std::nullopt;
    }

    if (target->checksum != calculate_checksum(*target)) {
        return std::nullopt;
    }

    protocol::PersistentConfig out{};
    std::memcpy(&out, target, sizeof(protocol::PersistentConfig));
    return out;
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
    defaults.display_cfg = protocol::DisplayConfig{
        .mode = protocol::DisplayMode::HwMonitor,
        .orientation = 0,
        .brightness = 80
    };

    return write(defaults);
}

} // namespace zdc::storage
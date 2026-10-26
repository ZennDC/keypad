#include "flash_storage.hpp"

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
#include "hardware/flash.h"
#include "hardware/sync.h"
#define FLASH_STORAGE_ON_DEVICE 1
#else
#define FLASH_STORAGE_ON_DEVICE 0
#endif

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace zdc::storage {

namespace {

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

// V2 legacy struct for backwards compatibility migration (single-profile layout)
struct alignas(4) PersistentConfigV2 {
    uint32_t magic{protocol::FlashConfigMagic};
    uint8_t version{2};
    protocol::DisplayConfig display_cfg{};
    protocol::KeyConfig keys[protocol::NumMacroKeys]{};
    uint8_t checksum{0};
};

uint8_t calculate_checksum_v2(const PersistentConfigV2& cfg) {
    uint8_t sum = 0;
    const auto* bytes = reinterpret_cast<const uint8_t*>(&cfg);
    constexpr size_t len = offsetof(PersistentConfigV2, checksum);
    for (size_t i = 0; i < len; ++i) {
        sum += bytes[i];
    }
    return sum;
}

} // namespace

#if FLASH_STORAGE_ON_DEVICE
// Reserve the very last 4096-byte sector of onboard flash
constexpr uint32_t TargetOffset = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

const protocol::PersistentConfig* FlashStorage::flash_target() {
    return reinterpret_cast<const protocol::PersistentConfig*>(XIP_BASE + TargetOffset);
}

bool FlashStorage::write_internal(const uint8_t* data, size_t size) {
    constexpr size_t FlashSectorSize = FLASH_SECTOR_SIZE;
    constexpr size_t FlashPageSize = FLASH_PAGE_SIZE;
    size_t program_size = ((size + FlashPageSize - 1) / FlashPageSize) * FlashPageSize;

    alignas(4) uint8_t buffer[FlashSectorSize];
    std::memset(buffer, 0xFF, sizeof(buffer));
    std::memcpy(buffer, data, size);

    // Disable interrupts before modifying XIP flash
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(TargetOffset, FlashSectorSize);
    flash_range_program(TargetOffset, buffer, program_size);
    restore_interrupts(interrupts);

    return true;
}
#else
constexpr size_t SimulatedSectorSize = 4096;
alignas(4) static uint8_t s_simulated_flash[SimulatedSectorSize];
static bool s_flash_initialized = false;

const protocol::PersistentConfig* FlashStorage::flash_target() {
    if (!s_flash_initialized) {
        std::memset(s_simulated_flash, 0xFF, sizeof(s_simulated_flash));
        s_flash_initialized = true;
    }
    return reinterpret_cast<const protocol::PersistentConfig*>(s_simulated_flash);
}

bool FlashStorage::write_internal(const uint8_t* data, size_t size) {
    std::memset(s_simulated_flash, 0xFF, sizeof(s_simulated_flash));
    std::memcpy(s_simulated_flash, data, size);
    s_flash_initialized = true;
    return true;
}

void FlashStorage::test_set_flash_raw(const void* data, size_t size) {
    std::memset(s_simulated_flash, 0xFF, sizeof(s_simulated_flash));
    if (data && size > 0) {
        std::memcpy(s_simulated_flash, data, std::min(size, SimulatedSectorSize));
    }
    s_flash_initialized = true;
}

void FlashStorage::test_reset_flash() {
    std::memset(s_simulated_flash, 0xFF, sizeof(s_simulated_flash));
    s_flash_initialized = false;
}
#endif

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
        if (target->profile_count == 0 || target->profile_count > protocol::MaxProfiles) {
            return std::nullopt;
        }
        if (target->checksum != calculate_checksum(*target)) {
            return std::nullopt;
        }

        protocol::PersistentConfig out{};
        std::memcpy(&out, target, sizeof(protocol::PersistentConfig));
        return out;
    }

    // Backwards compatibility: load and migrate version 2 layout (single profile)
    if (target->version == 2) {
        const auto* target_v2 = reinterpret_cast<const PersistentConfigV2*>(target);
        if (target_v2->checksum != calculate_checksum_v2(*target_v2)) {
            return std::nullopt;
        }

        protocol::PersistentConfig out{};
        out.magic = protocol::FlashConfigMagic;
        out.version = protocol::FlashConfigVersion;
        out.display_cfg = target_v2->display_cfg;
        out.profile_count = 1;
        out.active_profile = 0;
        out.reserved[0] = 0;
        out.reserved[1] = 0;
        std::strncpy(out.profiles[0].name, "Default", protocol::ProfileNameLength - 1);
        std::memcpy(out.profiles[0].keys, target_v2->keys, sizeof(target_v2->keys));
        out.checksum = calculate_checksum(out);
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
        out.profile_count = 1;
        out.active_profile = 0;
        out.reserved[0] = 0;
        out.reserved[1] = 0;
        std::strncpy(out.profiles[0].name, "Default", protocol::ProfileNameLength - 1);
        for (size_t i = 0; i < protocol::NumMacroKeys; ++i) {
            out.profiles[0].keys[i].report_type = target_v1->keys[i].report_type;
            out.profiles[0].keys[i].binding_type = protocol::BindingType::Normal;
            out.profiles[0].keys[i].usage_code = target_v1->keys[i].usage_code;
        }
        out.checksum = calculate_checksum(out);
        return out;
    }

    return std::nullopt;
}

bool FlashStorage::write(const protocol::PersistentConfig& config) {
    if (config.profile_count == 0 || config.profile_count > protocol::MaxProfiles) {
        return false;
    }

    protocol::PersistentConfig staged{};
    std::memcpy(&staged, &config, sizeof(protocol::PersistentConfig));

    staged.magic = protocol::FlashConfigMagic;
    staged.version = protocol::FlashConfigVersion;
    staged.reserved[0] = 0;
    staged.reserved[1] = 0;
    if (staged.active_profile >= staged.profile_count) {
        staged.active_profile = 0;
    }
    staged.checksum = calculate_checksum(staged);

    return write_internal(reinterpret_cast<const uint8_t*>(&staged), sizeof(staged));
}

bool FlashStorage::reset_defaults() {
    protocol::PersistentConfig defaults{};
    defaults.magic = protocol::FlashConfigMagic;
    defaults.version = protocol::FlashConfigVersion;
    defaults.display_cfg =
        protocol::DisplayConfig{.mode = protocol::DisplayMode::HwMonitor, .orientation = 0, .brightness = 80};
    defaults.profile_count = 1;
    defaults.active_profile = 0;
    defaults.reserved[0] = 0;
    defaults.reserved[1] = 0;
    std::strncpy(defaults.profiles[0].name, "Default", protocol::ProfileNameLength - 1);

    return write(defaults);
}

size_t FlashStorage::profile_count() const {
    auto stored = read();
    return stored.has_value() ? stored->profile_count : 0;
}

uint8_t FlashStorage::active_profile_index() const {
    auto stored = read();
    return stored.has_value() ? stored->active_profile : 0;
}

bool FlashStorage::set_active_profile_index(uint8_t index) {
    auto stored = read();
    if (!stored.has_value() || index >= stored->profile_count) {
        return false;
    }
    stored->active_profile = index;
    return write(*stored);
}

std::optional<protocol::ProfileConfig> FlashStorage::get_profile(size_t index) const {
    auto stored = read();
    if (!stored.has_value() || index >= stored->profile_count) {
        return std::nullopt;
    }
    return stored->profiles[index];
}

bool FlashStorage::set_profile(size_t index, const protocol::ProfileConfig& profile) {
    auto stored = read();
    if (!stored.has_value() || index >= stored->profile_count) {
        return false;
    }
    stored->profiles[index] = profile;
    return write(*stored);
}

bool FlashStorage::add_profile(const protocol::ProfileConfig& profile) {
    auto stored = read();
    if (!stored.has_value() || stored->profile_count >= protocol::MaxProfiles) {
        return false;
    }
    stored->profiles[stored->profile_count] = profile;
    stored->profile_count++;
    return write(*stored);
}

bool FlashStorage::remove_profile(size_t index) {
    auto stored = read();
    if (!stored.has_value() || stored->profile_count <= 1 || index >= stored->profile_count) {
        return false;
    }

    for (size_t i = index; i + 1 < stored->profile_count; ++i) {
        stored->profiles[i] = stored->profiles[i + 1];
    }
    std::memset(&stored->profiles[stored->profile_count - 1], 0, sizeof(protocol::ProfileConfig));
    stored->profile_count--;

    if (stored->active_profile >= stored->profile_count) {
        stored->active_profile = static_cast<uint8_t>(stored->profile_count - 1);
    }

    return write(*stored);
}

} // namespace zdc::storage
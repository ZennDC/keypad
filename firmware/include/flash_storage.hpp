#pragma once

#include "protocol.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace zdc::storage {

class FlashStorage {
public:
    static FlashStorage& instance() {
        static FlashStorage inst;
        return inst;
    }

    // Checks flash validity; writes defaults if corrupt or uninitialized
    void init();

    // Returns populated PersistentConfig if valid, std::nullopt otherwise
    [[nodiscard]] std::optional<protocol::PersistentConfig> read() const;

    // Erases sector and programs configuration pages
    bool write(const protocol::PersistentConfig& config);

    // Resets flash storage to default values
    bool reset_defaults();

    // Profile management API (variable amount of profiles)
    [[nodiscard]] size_t profile_count() const;
    [[nodiscard]] uint8_t active_profile_index() const;
    bool set_active_profile_index(uint8_t index);

    [[nodiscard]] std::optional<protocol::ProfileConfig> get_profile(size_t index) const;
    bool set_profile(size_t index, const protocol::ProfileConfig& profile);
    bool add_profile(const protocol::ProfileConfig& profile);
    bool remove_profile(size_t index);

#if !defined(PICO_ON_DEVICE) || !PICO_ON_DEVICE
    // Test helper for host unit tests: raw flash injection and reset
    static void test_set_flash_raw(const void* data, size_t size);
    static void test_reset_flash();
#endif

private:
    FlashStorage() = default;
    FlashStorage(const FlashStorage&) = delete;
    FlashStorage& operator=(const FlashStorage&) = delete;

    [[nodiscard]] static uint8_t calculate_checksum(const protocol::PersistentConfig& cfg);
    [[nodiscard]] static const protocol::PersistentConfig* flash_target();
    static bool write_internal(const uint8_t* data, size_t size);
};

} // namespace zdc::storage
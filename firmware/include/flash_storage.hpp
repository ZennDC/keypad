#pragma once

#include "protocol.hpp"
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

    // Erases last sector and programs configuration page
    bool write(const protocol::PersistentConfig& config);

    // Resets flash storage to default values
    bool reset_defaults();

private:
    FlashStorage() = default;
    FlashStorage(const FlashStorage&) = delete;
    FlashStorage& operator=(const FlashStorage&) = delete;

    [[nodiscard]] static uint8_t calculate_checksum(const protocol::PersistentConfig& cfg);
    [[nodiscard]] static const protocol::PersistentConfig* flash_target();
};

} // namespace zdc::storage
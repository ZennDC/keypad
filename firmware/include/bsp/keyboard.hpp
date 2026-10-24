#pragma once

#include <cstddef>
#include <stdint.h>

namespace bsp {

// Logische identifier voor alle toetsen op het bord
enum class Key : uint8_t {
    // 4x4 Matrix (Rij 0..3, Kolom 0..3)
    M1_1 = 0,
    M1_2,
    M1_3,
    M1_4,
    M2_1,
    M2_2,
    M2_3,
    M2_4,
    M3_1,
    M3_2,
    M3_3,
    M3_4,
    M4_1,
    M4_2,
    M4_3,
    M4_4,

    // Standalone knoppen op GP28 en GP29
    SideBtn1, // GP29
    SideBtn2, // GP28

    Count
};

class Keyboard {
public:
    Keyboard() = default;

    // Configureert GPIO's (matrix kolommen als output, rijen/knoppen als input met pull-up)
    void init();

    // Scant de matrix en losse knoppen en past debouncing toe (aanroepen in main loop)
    void update();

    // Toestand opvragen
    [[nodiscard]] bool is_down(Key key) const;
    [[nodiscard]] bool just_pressed(Key key) const;
    [[nodiscard]] bool just_released(Key key) const;

    // Helper om direct een raw bitmasker te krijgen van alle actieve toetsen
    [[nodiscard]] uint32_t state_mask() const { return current_state_; }

private:
    uint32_t current_state_{0};  // Gedebouncete huidige toestand (1 bit per toets)
    uint32_t previous_state_{0}; // Vorige toestand voor edge-detection
    uint32_t raw_history_[4]{0}; // Ringbuffer/geschiedenis voor debouncen
    uint8_t history_idx_{0};

    uint32_t scan_raw();
};

} // namespace bsp
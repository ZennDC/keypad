#include "bsp/keyboard.hpp"
#include "bsp/board_config.hpp"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

namespace bsp {

using namespace bsp::config::matrix;
using namespace bsp::config::buttons;

void Keyboard::init() {
    // 1. ROWS driven as OUTPUT (Idle HIGH)
    for (uint pin : row_pins) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 1);
    }

    // 2. COLS read as INPUT with pull-up (Active LOW)
    for (uint pin : col_pins) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    // 3. Standalone buttons
    gpio_init(btn_side_1_pin);
    gpio_set_dir(btn_side_1_pin, GPIO_IN);
    gpio_pull_up(btn_side_1_pin);

    gpio_init(btn_side_2_pin);
    gpio_set_dir(btn_side_2_pin, GPIO_IN);
    gpio_pull_up(btn_side_2_pin);
}

uint32_t Keyboard::scan_raw() {
    uint32_t raw = 0;

    // Drive each ROW LOW
    for (size_t r = 0; r < num_rows; ++r) {
        gpio_put(row_pins[r], 0);
        sleep_us(settle_time_us);

        // Read all COLS
        for (size_t c = 0; c < num_cols; ++c) {
            if (!gpio_get(col_pins[c])) {
                size_t key_index = r * num_cols + c;
                raw |= (1UL << key_index);
            }
        }

        gpio_put(row_pins[r], 1);
    }

    // Standalone buttons
    if (!gpio_get(btn_side_1_pin)) {
        raw |= (1UL << static_cast<uint8_t>(Key::SideBtn1));
    }
    if (!gpio_get(btn_side_2_pin)) {
        raw |= (1UL << static_cast<uint8_t>(Key::SideBtn2));
    }

    return raw;
}

void Keyboard::update() {
    previous_state_ = current_state_;

    // Sla raw scan op in debouncing geschiedenis
    raw_history_[history_idx_] = scan_raw();
    history_idx_ = (history_idx_ + 1) % 4;

    // Een toets is alleen stabiel HIGH als alle 4 de laatste metingen 1 waren
    // en valt alleen af als alle 4 metingen 0 waren
    uint32_t stable_pressed = raw_history_[0] & raw_history_[1] & raw_history_[2] & raw_history_[3];
    uint32_t stable_released = ~(raw_history_[0] | raw_history_[1] | raw_history_[2] | raw_history_[3]);

    current_state_ |= stable_pressed;
    current_state_ &= ~stable_released;
}

bool Keyboard::is_down(Key key) const {
    if (key >= Key::Count) return false;
    return (current_state_ & (1UL << static_cast<uint8_t>(key))) != 0;
}

bool Keyboard::just_pressed(Key key) const {
    if (key >= Key::Count) return false;
    uint32_t bit = (1UL << static_cast<uint8_t>(key));
    return (current_state_ & bit) && !(previous_state_ & bit);
}

bool Keyboard::just_released(Key key) const {
    if (key >= Key::Count) return false;
    uint32_t bit = (1UL << static_cast<uint8_t>(key));
    return !(current_state_ & bit) && (previous_state_ & bit);
}

} // namespace bsp
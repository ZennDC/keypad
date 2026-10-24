#pragma once

#include "bsp/bsp.hpp"
#include <cstdint>
#include <string_view>

namespace ui {

enum class UIState : uint8_t { Normal, SettingsMenu, BootloaderCountdown, FactoryResetCountdown };

enum class MenuItem : uint8_t { Brightness = 0, SleepTimeout, Bootloader, ExitSave, Count };

class MenuController {
public:
    static MenuController& instance() {
        static MenuController inst;
        return inst;
    }

    void init(bsp::Board& board);

    // Call on every main loop iteration
    void update(bsp::Board& board);

    // Force complete redraw of main placeholder UI
    void draw_main_ui(bsp::Display& display);

    // Redraw a single key slot on main UI
    void draw_key_slot(bsp::Display& display, uint8_t key_idx, bool pressed);

    [[nodiscard]] UIState state() const { return state_; }
    [[nodiscard]] bool is_menu_active() const { return state_ != UIState::Normal; }
    [[nodiscard]] bool is_sleeping() const { return is_sleeping_; }

private:
    MenuController() = default;

    // Release-based gesture processor
    void process_side_buttons(bsp::Board& board, uint32_t now);

    // Trigger actions upon button release
    void on_btn1_released(bsp::Board& board);
    void on_btn2_released(bsp::Board& board);
    void on_chord_released(bsp::Board& board);

    // Flicker-free rendering helpers
    void render_full_menu(bsp::Display& display);
    void render_menu_row(bsp::Display& display, MenuItem item, bool selected);
    void render_modal_frame(bsp::Display& display, const char* title, uint16_t theme_color);
    void update_modal_progress(bsp::Display& display, const char* action, uint32_t remaining_s, float progress,
                               uint16_t theme_color);

    void apply_brightness(bsp::Board& board);
    void check_sleep_timer(bsp::Board& board, uint32_t now);
    void wake_display(bsp::Board& board, uint32_t now);

    UIState state_{UIState::Normal};
    MenuItem current_item_{MenuItem::Brightness};

    // Release-based button gesture tracking
    bool btn1_down_{false};
    bool btn2_down_{false};
    bool btn1_was_pressed_{false};
    bool btn2_was_pressed_{false};
    bool chord_detected_{false};
    bool hold_triggered_{false};
    uint32_t gesture_start_ms_{0};
    uint32_t chord_start_ms_{0};

    // Modal flicker-free state
    bool modal_frame_drawn_{false};
    uint32_t prev_remaining_s_{0xFFFFFFFF};
    int prev_fill_w_{-1};

    // Activity & sleep management
    uint32_t last_activity_ms_{0};
    bool is_sleeping_{false};
    uint8_t sleep_timeout_idx_{0}; // 0: Off, 1: 1m, 2: 5m, 3: 15m

    // Cached temporary menu adjustments
    uint8_t temp_brightness_{80};
    uint32_t menu_last_interaction_ms_{0};
};

} // namespace ui

#include "menu_controller.hpp"
#include "bsp/board_config.hpp"
#include "bsp/color.hpp"
#include "config_manager.hpp"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include <algorithm>
#include <cstdio>
#include <string_view>

using bsp::graphics::Color;

namespace ui {

namespace {

constexpr int MainStartX = 70;
constexpr int MainStartY = 70;
constexpr int MainBtnW = 40;
constexpr int MainBtnH = 36;
constexpr int MainGap = 6;

// Menu overlay geometry (center of 320x240 screen)
constexpr int MenuX = 40;
constexpr int MenuY = 36;
constexpr int MenuW = 240;
constexpr int MenuH = 160;

// Countdown modal geometry
constexpr int ModalX = 35;
constexpr int ModalY = 50;
constexpr int ModalW = 250;
constexpr int ModalH = 140;

constexpr int BarX = ModalX + 25;
constexpr int BarY = ModalY + 70;
constexpr int BarW = 200;
constexpr int BarH = 16;

const char* sleep_timeout_name(uint8_t idx) {
    switch (idx) {
    case 1:
        return "1 min";
    case 2:
        return "5 min";
    case 3:
        return "15 min";
    default:
        return "Off";
    }
}

uint32_t sleep_timeout_ms(uint8_t idx) {
    switch (idx) {
    case 1:
        return 60 * 1000;
    case 2:
        return 5 * 60 * 1000;
    case 3:
        return 15 * 60 * 1000;
    default:
        return 0;
    }
}

} // namespace

void MenuController::init(bsp::Board& board) {
    auto& cfg = config::ConfigManager::instance();
    temp_brightness_ = cfg.display_config().brightness;

    last_activity_ms_ = to_ms_since_boot(get_absolute_time());
    apply_brightness(board);
    draw_main_ui(board.display);
}

void MenuController::draw_key_slot(bsp::Display& display, uint8_t key_idx, bool pressed) {
    if (key_idx >= 16)
        return;

    const int row = key_idx / 4;
    const int col = key_idx % 4;
    const int x = MainStartX + col * (MainBtnW + MainGap);
    const int y = MainStartY + row * (MainBtnH + MainGap);

    if (pressed) {
        display.fill_rect(x, y, MainBtnW, MainBtnH, Color::from_rgb(0, 220, 255));
        display.fill_rect(x + 2, y + 2, MainBtnW - 4, MainBtnH - 4, Color::from_rgb(0, 100, 180));
    } else {
        display.fill_rect(x, y, MainBtnW, MainBtnH, Color::from_rgb(60, 65, 80));
        display.fill_rect(x + 2, y + 2, MainBtnW - 4, MainBtnH - 4, Color::from_rgb(20, 22, 30));
    }
}

void MenuController::draw_main_ui(bsp::Display& display) {
    display.clear(Color::Black);

    // Header bar (320x28)
    display.fill_rect(0, 0, 320, 28, Color::from_rgb(25, 30, 45));
    display.draw_text(10, 8, "ZDC MACROPAD - ACTIVE", Color::from_rgb(0, 220, 255), Color::from_rgb(25, 30, 45));

    // Profile indicator
    char profile_buf[32];
    auto& cfg = config::ConfigManager::instance();
    std::snprintf(profile_buf, sizeof(profile_buf), "Profile: %.*s", static_cast<int>(cfg.active_profile_name().size()),
                  cfg.active_profile_name().data());

    display.fill_rect(10, 38, 200, 12, Color::Black);
    display.draw_text(10, 38, profile_buf, Color::White, Color::Black);

    // USB Status
    display.draw_text(10, 52, "USB: Connected", Color::Green, Color::Black);

    // 4x4 visual button matrix representation
    for (uint8_t i = 0; i < 16; ++i) {
        draw_key_slot(display, i, false);
    }
}

void MenuController::render_menu_row(bsp::Display& display, MenuItem item, bool selected) {
    const int item_start_y = MenuY + 36;
    const int item_h = 24;
    const int item_y = item_start_y + static_cast<int>(item) * item_h;

    Color bg_color = selected ? Color::from_rgb(0, 80, 140) : Color::from_rgb(18, 22, 32);
    Color fg_color = selected ? Color::White : Color::from_rgb(180, 190, 205);

    display.fill_rect(MenuX + 6, item_y, MenuW - 12, item_h - 2, bg_color);

    char line_buf[40];
    switch (item) {
    case MenuItem::Brightness:
        std::snprintf(line_buf, sizeof(line_buf), " Brightness:    %3u%%", temp_brightness_);
        break;
    case MenuItem::SleepTimeout:
        std::snprintf(line_buf, sizeof(line_buf), " Sleep:         %s", sleep_timeout_name(sleep_timeout_idx_));
        break;
    case MenuItem::Bootloader:
        std::snprintf(line_buf, sizeof(line_buf), " Flash Mode (BOOTSEL)");
        break;
    case MenuItem::ExitSave:
        std::snprintf(line_buf, sizeof(line_buf), " Save & Exit");
        break;
    default:
        line_buf[0] = '\0';
        break;
    }

    display.draw_text(MenuX + 10, item_y + 5, line_buf, fg_color, bg_color);
}

void MenuController::render_full_menu(bsp::Display& display) {
    // Background card with shadow effect
    display.fill_rect(MenuX + 4, MenuY + 4, MenuW, MenuH, Color::from_rgb(5, 8, 12));
    display.fill_rect(MenuX, MenuY, MenuW, MenuH, Color::from_rgb(18, 22, 32));

    // Title banner
    display.fill_rect(MenuX, MenuY, MenuW, 26, Color::from_rgb(0, 110, 190));
    display.draw_text(MenuX + 65, MenuY + 7, "SETTINGS MENU", Color::White, Color::from_rgb(0, 110, 190));

    // Outer border
    display.fill_rect(MenuX, MenuY, MenuW, 1, Color::from_rgb(0, 180, 255));
    display.fill_rect(MenuX, MenuY + MenuH - 1, MenuW, 1, Color::from_rgb(0, 180, 255));
    display.fill_rect(MenuX, MenuY, 1, MenuH, Color::from_rgb(0, 180, 255));
    display.fill_rect(MenuX + MenuW - 1, MenuY, 1, MenuH, Color::from_rgb(0, 180, 255));

    // Draw all rows
    for (uint8_t i = 0; i < static_cast<uint8_t>(MenuItem::Count); ++i) {
        auto item = static_cast<MenuItem>(i);
        render_menu_row(display, item, item == current_item_);
    }

    // Footer navigation hint (Btn1 is UP, Btn2 is DOWN)
    display.fill_rect(MenuX + 6, MenuY + MenuH - 22, MenuW - 12, 16, Color::from_rgb(12, 15, 22));
    display.draw_text(MenuX + 16, MenuY + MenuH - 18, "B1:Up B2:Dn Both:Select", Color::from_rgb(100, 120, 145),
                      Color::from_rgb(12, 15, 22));
}

void MenuController::render_modal_frame(bsp::Display& display, const char* title, uint16_t theme_color) {
    Color theme = Color{theme_color};

    // Modal background & frame (drawn ONCE)
    display.fill_rect(ModalX + 4, ModalY + 4, ModalW, ModalH, Color::from_rgb(5, 5, 8));
    display.fill_rect(ModalX, ModalY, ModalW, ModalH, Color::from_rgb(16, 18, 24));

    // Title banner
    display.fill_rect(ModalX, ModalY, ModalW, 26, theme);
    display.draw_text(ModalX + 25, ModalY + 7, title, Color::White, theme);

    // Border
    display.fill_rect(ModalX, ModalY, ModalW, 1, theme);
    display.fill_rect(ModalX, ModalY + ModalH - 1, ModalW, 1, theme);
    display.fill_rect(ModalX, ModalY, 1, ModalH, theme);
    display.fill_rect(ModalX + ModalW - 1, ModalY, 1, ModalH, theme);

    // Progress bar frame
    display.fill_rect(BarX - 1, BarY - 1, BarW + 2, BarH + 2, theme);
    display.fill_rect(BarX, BarY, BarW, BarH, Color::from_rgb(10, 12, 16));

    // Cancel hint
    display.draw_text(ModalX + 32, ModalY + 104, "(Release buttons to cancel)", Color::from_rgb(160, 170, 185),
                      Color::from_rgb(16, 18, 24));
}

void MenuController::update_modal_progress(bsp::Display& display, const char* action, uint32_t remaining_s,
                                           float progress, uint16_t theme_color) {
    Color theme = Color{theme_color};

    // Update countdown text ONLY when remaining_s changes
    if (remaining_s != prev_remaining_s_) {
        prev_remaining_s_ = remaining_s;
        display.fill_rect(ModalX + 25, ModalY + 44, 200, 16, Color::from_rgb(16, 18, 24));

        char status_buf[48];
        std::snprintf(status_buf, sizeof(status_buf), "%s: %us", action, static_cast<unsigned int>(remaining_s));
        display.draw_text(ModalX + 30, ModalY + 44, status_buf, Color::White, Color::from_rgb(16, 18, 24));
    }

    // Update progress bar ONLY advancing forward
    int fill_w = std::clamp(static_cast<int>(progress * BarW), 0, BarW);
    if (fill_w != prev_fill_w_) {
        if (prev_fill_w_ < 0) {
            display.fill_rect(BarX, BarY, fill_w, BarH, theme);
        } else if (fill_w > prev_fill_w_) {
            display.fill_rect(BarX + prev_fill_w_, BarY, fill_w - prev_fill_w_, BarH, theme);
        }
        prev_fill_w_ = fill_w;
    }
}

void MenuController::apply_brightness(bsp::Board& board) {
    board.display.set_backlight(temp_brightness_);
}

void MenuController::wake_display(bsp::Board& board, uint32_t now) {
    if (is_sleeping_) {
        is_sleeping_ = false;
        board.display.set_sleep(false);
        apply_brightness(board);
        draw_main_ui(board.display);
    }
    last_activity_ms_ = now;
}

void MenuController::check_sleep_timer(bsp::Board& board, uint32_t now) {
    uint32_t timeout = sleep_timeout_ms(sleep_timeout_idx_);
    if (timeout == 0)
        return;

    if (!is_sleeping_ && (now - last_activity_ms_ >= timeout)) {
        is_sleeping_ = true;
        board.display.set_sleep(true);
    }
}

void MenuController::on_chord_released(bsp::Board& board) {
    if (state_ == UIState::Normal) {
        state_ = UIState::SettingsMenu;
        menu_last_interaction_ms_ = to_ms_since_boot(get_absolute_time());
        render_full_menu(board.display);
        return;
    }

    if (state_ == UIState::SettingsMenu) {
        menu_last_interaction_ms_ = to_ms_since_boot(get_absolute_time());

        switch (current_item_) {
        case MenuItem::Brightness:
            temp_brightness_ = (temp_brightness_ >= 100) ? 20 : (temp_brightness_ + 20);
            apply_brightness(board);
            render_menu_row(board.display, current_item_, true);
            break;

        case MenuItem::SleepTimeout:
            sleep_timeout_idx_ = (sleep_timeout_idx_ + 1) % 4;
            render_menu_row(board.display, current_item_, true);
            break;

        case MenuItem::Bootloader:
            // Direct jump from menu item
            board.display.clear(Color::from_rgb(0, 100, 200));
            board.display.draw_text(60, 110, "REBOOTING TO UF2...", Color::White, Color::from_rgb(0, 100, 200), 2);
            sleep_ms(300);
            reset_usb_boot(0, 0);
            return;

        case MenuItem::ExitSave: {
            auto& cfg = config::ConfigManager::instance();
            cfg.display_config().brightness = temp_brightness_;
            cfg.save_to_flash();

            state_ = UIState::Normal;
            draw_main_ui(board.display);
            return;
        }

        default:
            break;
        }
    }
}

// Button 1 is UP (Inverted)
void MenuController::on_btn1_released(bsp::Board& board) {
    if (state_ == UIState::Normal) {
        auto& cfg = config::ConfigManager::instance();
        cfg.prev_profile(); // Inverted: Btn 1 cycles UP / Backward
        draw_main_ui(board.display);
        return;
    }

    if (state_ == UIState::SettingsMenu) {
        menu_last_interaction_ms_ = to_ms_since_boot(get_absolute_time());

        MenuItem old_item = current_item_;
        uint8_t prev = (static_cast<uint8_t>(current_item_) == 0) ? (static_cast<uint8_t>(MenuItem::Count) - 1)
                                                                  : (static_cast<uint8_t>(current_item_) - 1);
        current_item_ = static_cast<MenuItem>(prev);

        // Flicker-free row swap
        render_menu_row(board.display, old_item, false);
        render_menu_row(board.display, current_item_, true);
    }
}

// Button 2 is DOWN (Inverted)
void MenuController::on_btn2_released(bsp::Board& board) {
    if (state_ == UIState::Normal) {
        auto& cfg = config::ConfigManager::instance();
        cfg.next_profile(); // Inverted: Btn 2 cycles DOWN / Forward
        draw_main_ui(board.display);
        return;
    }

    if (state_ == UIState::SettingsMenu) {
        menu_last_interaction_ms_ = to_ms_since_boot(get_absolute_time());

        MenuItem old_item = current_item_;
        uint8_t next = (static_cast<uint8_t>(current_item_) + 1) % static_cast<uint8_t>(MenuItem::Count);
        current_item_ = static_cast<MenuItem>(next);

        // Flicker-free row swap
        render_menu_row(board.display, old_item, false);
        render_menu_row(board.display, current_item_, true);
    }
}

void MenuController::process_side_buttons(bsp::Board& board, uint32_t now) {
    const bool b1 = board.keys.is_down(bsp::Key::SideBtn1);
    const bool b2 = board.keys.is_down(bsp::Key::SideBtn2);

    // Any button active: wake display & manage gesture
    if (b1 || b2) {
        wake_display(board, now);

        // First button down of this gesture
        if (gesture_start_ms_ == 0) {
            gesture_start_ms_ = now;
            btn1_was_pressed_ = b1;
            btn2_was_pressed_ = b2;
            chord_detected_ = (b1 && b2);
            chord_start_ms_ = (b1 && b2) ? now : 0;
            hold_triggered_ = false;
        }

        if (b1)
            btn1_was_pressed_ = true;
        if (b2)
            btn2_was_pressed_ = true;

        if (b1 && b2) {
            chord_detected_ = true;
            if (chord_start_ms_ == 0) {
                chord_start_ms_ = now;
            }
        }

        // --- LONG HOLD CHECKS (Factory Reset & Bootloader) ---

        // 1. Both buttons held for Factory Reset
        if (b1 && b2 && chord_detected_) {
            const uint32_t chord_held = now - chord_start_ms_;

            if (chord_held >= 1500) {
                hold_triggered_ = true;
                state_ = UIState::FactoryResetCountdown;

                if (!modal_frame_drawn_) {
                    modal_frame_drawn_ = true;
                    prev_remaining_s_ = 0xFFFFFFFF;
                    prev_fill_w_ = -1;
                    render_modal_frame(board.display, "  FACTORY RESET  ", Color::from_rgb(235, 40, 40).value);
                }

                if (chord_held >= 5000) {
                    board.display.clear(Color::from_rgb(180, 0, 0));
                    board.display.draw_text(50, 110, "FACTORY RESETTING...", Color::White, Color::from_rgb(180, 0, 0),
                                            2);
                    config::ConfigManager::instance().reset_to_factory_defaults();
                    sleep_ms(500);
                    watchdog_reboot(0, 0, 0);
                    return;
                }

                uint32_t remaining_s = (5000 - chord_held + 999) / 1000;
                float progress = static_cast<float>(chord_held - 1500) / 3500.0f;
                update_modal_progress(board.display, "Wiping flash in", remaining_s, progress,
                                      Color::from_rgb(235, 40, 40).value);
                return;
            }
        }

        // 2. Button 1 alone held for Bootloader / Flash Mode (only in normal mode)
        if (b1 && !b2 && !chord_detected_ && (state_ == UIState::Normal || state_ == UIState::BootloaderCountdown)) {
            const uint32_t b1_held = now - gesture_start_ms_;

            if (b1_held >= 1000) {
                hold_triggered_ = true;
                state_ = UIState::BootloaderCountdown;

                if (!modal_frame_drawn_) {
                    modal_frame_drawn_ = true;
                    prev_remaining_s_ = 0xFFFFFFFF;
                    prev_fill_w_ = -1;
                    render_modal_frame(board.display, "ENTERING FLASH MODE", Color::from_rgb(255, 170, 0).value);
                }

                if (b1_held >= 3500) {
                    board.display.clear(Color::from_rgb(0, 100, 200));
                    board.display.draw_text(60, 110, "REBOOTING TO UF2...", Color::White, Color::from_rgb(0, 100, 200),
                                            2);
                    sleep_ms(300);
                    reset_usb_boot(0, 0);
                    return;
                }

                uint32_t remaining_s = (3500 - b1_held + 999) / 1000;
                float progress = static_cast<float>(b1_held - 1000) / 2500.0f;
                update_modal_progress(board.display, "BOOTSEL in", remaining_s, progress,
                                      Color::from_rgb(255, 170, 0).value);
                return;
            }
        }
    } else {
        // --- BUTTONS RELEASED (FIRE GESTURE ACTION) ---
        if (gesture_start_ms_ > 0) {
            if (hold_triggered_) {
                // User released buttons before countdown completed -> clean cancellation
                modal_frame_drawn_ = false;
                state_ = UIState::Normal;
                draw_main_ui(board.display);
            } else {
                // Short press gesture resolved strictly on release
                if (chord_detected_) {
                    on_chord_released(board);
                } else if (btn1_was_pressed_) {
                    on_btn1_released(board);
                } else if (btn2_was_pressed_) {
                    on_btn2_released(board);
                }
            }

            // Reset gesture tracker
            gesture_start_ms_ = 0;
            chord_start_ms_ = 0;
            btn1_was_pressed_ = false;
            btn2_was_pressed_ = false;
            chord_detected_ = false;
            hold_triggered_ = false;
            modal_frame_drawn_ = false;
        }
    }
}

void MenuController::update(bsp::Board& board) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());

    // Check if any key matrix keys are active -> reset sleep timer
    for (uint8_t i = 0; i < 16; ++i) {
        if (board.keys.is_down(static_cast<bsp::Key>(i))) {
            wake_display(board, now);
            break;
        }
    }

    check_sleep_timer(board, now);

    // Auto-exit Settings Menu after 8s inactivity
    if (state_ == UIState::SettingsMenu && (now - menu_last_interaction_ms_ >= 8000)) {
        auto& cfg = config::ConfigManager::instance();
        cfg.display_config().brightness = temp_brightness_;
        cfg.save_to_flash();

        state_ = UIState::Normal;
        draw_main_ui(board.display);
    }

    process_side_buttons(board, now);
}

} // namespace ui

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "bsp/bsp.hpp"
#include "bsp/color.hpp"
#include "config_manager.hpp"
#include "protocol.hpp"

using bsp::graphics::Color;
using namespace zdc::protocol;

namespace {

constexpr int StartX = 70;
constexpr int StartY = 70;
constexpr int BtnW = 40;
constexpr int BtnH = 36;
constexpr int Gap = 6;

void draw_key_slot(bsp::Display& display, uint8_t key_idx, bool pressed) {
    if (key_idx >= 16)
        return;

    const int row = key_idx / 4;
    const int col = key_idx % 4;
    const int x = StartX + col * (BtnW + Gap);
    const int y = StartY + row * (BtnH + Gap);

    if (pressed) {
        display.fill_rect(x, y, BtnW, BtnH, Color::from_rgb(0, 220, 255));
        display.fill_rect(x + 2, y + 2, BtnW - 4, BtnH - 4, Color::from_rgb(0, 100, 180));
    } else {
        display.fill_rect(x, y, BtnW, BtnH, Color::from_rgb(60, 65, 80));
        display.fill_rect(x + 2, y + 2, BtnW - 4, BtnH - 4, Color::from_rgb(20, 22, 30));
    }
}

void draw_placeholder_ui(bsp::Display& display) {
    display.clear(Color::Black);

    // Header bar (320x28)
    display.fill_rect(0, 0, 320, 28, Color::from_rgb(25, 30, 45));
    display.draw_text(10, 8, "ZDC MACROPAD - ACTIVE", Color::from_rgb(0, 220, 255), Color::from_rgb(25, 30, 45));

    // Status indicators
    display.draw_text(10, 38, "Profile: Default", Color::White, Color::Black);
    display.draw_text(10, 52, "USB: Connected", Color::Green, Color::Black);

    // Initial 4x4 visual button representation
    for (uint8_t i = 0; i < 16; ++i) {
        draw_key_slot(display, i, false);
    }
}

static void process_keyboard_events(bsp::Board& board) {
    auto& cfg_mgr = config::ConfigManager::instance();
    MacroKey* keys = cfg_mgr.keys_data();

    bool state_changed = false;
    bool consumer_released = false;

    // 1. Process visual and debounce updates for ALL keys
    for (uint8_t i = 0; i < NumMacroKeys; ++i) {
        auto key_id = static_cast<bsp::Key>(i);

        if (board.keys.just_pressed(key_id)) {
            keys[i].was_pressed = true;
            draw_key_slot(board.display, i, true);
            state_changed = true;
        } else if (board.keys.just_released(key_id)) {
            keys[i].was_pressed = false;
            draw_key_slot(board.display, i, false);
            state_changed = true;
            if (keys[i].config.report_type == ReportType::Consumer) {
                consumer_released = true;
            }
        }
    }

    if (!state_changed || !tud_hid_ready())
        return;

    // 2. Scan active keys
    uint8_t keycodes[6] = {0};
    uint8_t keycode_idx = 0;
    uint16_t consumer_code = 0;
    bool has_consumer = false;

    for (uint8_t i = 0; i < NumMacroKeys; ++i) {
        if (board.keys.is_down(static_cast<bsp::Key>(i))) {
            const auto& cfg = keys[i].config;
            if (cfg.report_type == ReportType::Consumer) {
                consumer_code = cfg.usage_code;
                has_consumer = true;
            } else if (cfg.report_type == ReportType::Keyboard) {
                if (keycode_idx < 6) {
                    keycodes[keycode_idx++] = static_cast<uint8_t>(cfg.usage_code);
                }
            }
        }
    }

    // 3. Dispatch Consumer Report
    if (has_consumer) {
        tud_hid_report(static_cast<uint8_t>(ReportId::ConsumerControl), &consumer_code, sizeof(consumer_code));
    } else if (consumer_released) {
        // Send release (0) report so the OS stops repeating the media key!
        uint16_t empty = 0;
        tud_hid_report(static_cast<uint8_t>(ReportId::ConsumerControl), &empty, sizeof(empty));
    }

    // 4. Dispatch Keyboard Report
    // (Only send if keyboard keys are held or if keys transitioned)
    if (keycode_idx > 0 || !has_consumer) {
        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), 0, keycodes);
    }
}

} // namespace

int main(void) {
    board_init();

    // 1. Initialize BSP hardware (SPI Display, GPIO Matrix, Keys)
    auto& board = bsp::Board::instance();
    board.init();

    // 2. Configure and initialize persistent settings
    auto& cfg = config::ConfigManager::instance();

    cfg.set_on_brightness_changed([&board](uint8_t brightness) { board.display.set_backlight(brightness); });

    cfg.init();

    // 3. Set display backlight from persistent configuration
    board.display.set_backlight(cfg.display_config().brightness);

    // 4. Render main layout
    draw_placeholder_ui(board.display);

    // 5. Initialize TinyUSB device stack
    tusb_init();

    while (true) {
        tud_task();

        // Scan debounced matrix & side buttons
        board.update();

        // Dispatch HID packets and update key visual states on screen
        process_keyboard_events(board);
    }

    return 0;
}
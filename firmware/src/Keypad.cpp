#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "bsp/board_config.hpp"
#include "bsp/bsp.hpp"
#include "config_manager.hpp"
#include "menu_controller.hpp"
#include "protocol.hpp"

using namespace zdc::protocol;

namespace {

static void process_keyboard_events(bsp::Board& board) {
    auto& menu = ui::MenuController::instance();

    // While in settings menu, countdowns, or sleeping, suppress macro HID emissions
    if (menu.is_menu_active() || menu.is_sleeping()) {
        return;
    }

    auto& cfg_mgr = config::ConfigManager::instance();
    MacroKey* keys = cfg_mgr.keys_data();

    bool state_changed = false;
    bool consumer_released = false;

    // 1. Process visual and debounce updates for matrix keys (0..15)
    for (uint8_t i = 0; i < NumMacroKeys; ++i) {
        auto key_id = static_cast<bsp::Key>(i);

        if (board.keys.just_pressed(key_id)) {
            keys[i].was_pressed = true;
            menu.draw_key_slot(board.display, i, true);
            state_changed = true;
        } else if (board.keys.just_released(key_id)) {
            keys[i].was_pressed = false;
            menu.draw_key_slot(board.display, i, false);
            state_changed = true;
            const auto& key_cfg = cfg_mgr.get_active_key_config(i);
            if (key_cfg.report_type == ReportType::Consumer) {
                consumer_released = true;
            }
        }
    }

    if (!state_changed || !tud_hid_ready())
        return;

    // 2. Scan active keys from current active profile
    uint8_t keycodes[6] = {0};
    uint8_t keycode_idx = 0;
    uint16_t consumer_code = 0;
    bool has_consumer = false;

    for (uint8_t i = 0; i < NumMacroKeys; ++i) {
        if (board.keys.is_down(static_cast<bsp::Key>(i))) {
            const auto& key_cfg = cfg_mgr.get_active_key_config(i);
            if (key_cfg.report_type == ReportType::Consumer) {
                consumer_code = key_cfg.usage_code;
                has_consumer = true;
            } else if (key_cfg.report_type == ReportType::Keyboard) {
                if (keycode_idx < 6) {
                    keycodes[keycode_idx++] = static_cast<uint8_t>(key_cfg.usage_code);
                }
            }
        }
    }

    // 3. Dispatch Consumer Report
    if (has_consumer) {
        tud_hid_report(static_cast<uint8_t>(ReportId::ConsumerControl), &consumer_code, sizeof(consumer_code));
    } else if (consumer_released) {
        // Send release (0) report so the OS stops repeating the media key
        uint16_t empty = 0;
        tud_hid_report(static_cast<uint8_t>(ReportId::ConsumerControl), &empty, sizeof(empty));
    }

    // 4. Dispatch Keyboard Report
    if (keycode_idx > 0 || !has_consumer) {
        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), 0, keycodes);
    }
}

} // namespace

int main(void) {
    // 0. Early Bootloader Failsafe Check:
    // If SideBtn1 (GP28) is held during power-on / USB plug-in, jump straight to BOOTSEL
    gpio_init(bsp::config::buttons::btn_side_1_pin);
    gpio_set_dir(bsp::config::buttons::btn_side_1_pin, GPIO_IN);
    gpio_pull_up(bsp::config::buttons::btn_side_1_pin);
    sleep_us(150);
    if (!gpio_get(bsp::config::buttons::btn_side_1_pin)) {
        reset_usb_boot(0, 0);
    }

    board_init();

    // 1. Initialize BSP hardware (SPI Display, GPIO Matrix, Keys)
    auto& board = bsp::Board::instance();
    board.init();

    // 2. Configure and initialize persistent settings
    auto& cfg = config::ConfigManager::instance();

    cfg.set_on_brightness_changed([&board](uint8_t brightness) { board.display.set_backlight(brightness); });
    cfg.set_on_display_changed(
        [&board](const zdc::protocol::DisplayConfig& disp) { board.display.set_backlight(disp.brightness); });

    cfg.init();

    // Apply initial display configuration from Flash
    board.display.set_backlight(cfg.display_config().brightness);

    // 3. Initialize Menu & UI Controller
    auto& menu = ui::MenuController::instance();
    menu.init(board);

    // 4. Initialize TinyUSB device stack
    tusb_init();

    while (true) {
        tud_task();

        // Scan debounced matrix & side buttons
        board.update();

        // Update menu navigation, countdowns, and profile transitions
        menu.update(board);

        // Dispatch HID packets and update key visual states on screen
        process_keyboard_events(board);
    }

    return 0;
}

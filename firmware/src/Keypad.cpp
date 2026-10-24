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

#include "tap_dance.hpp"

using namespace zdc::protocol;
using namespace zdc::input;

namespace {

static TapDanceEngine g_tap_dance_engine;
static uint8_t g_last_profile = 0xFF;

static void sync_tap_dance_bindings(const config::ConfigManager& cfg_mgr) {
    for (uint8_t i = 0; i < NumMacroKeys; ++i) {
        const auto& key_cfg = cfg_mgr.get_active_key_config(i);
        if (key_cfg.binding_type == BindingType::TapDance) {
            g_tap_dance_engine.set_from_key_config(i, key_cfg);
        } else {
            g_tap_dance_engine.reset_key(i);
        }
    }
}

static void process_keyboard_events(bsp::Board& board) {
    auto& menu = ui::MenuController::instance();

    // While in settings menu, countdowns, or sleeping, suppress macro HID emissions
    if (menu.is_menu_active() || menu.is_sleeping()) {
        return;
    }

    auto& cfg_mgr = config::ConfigManager::instance();
    MacroKey* keys = cfg_mgr.keys_data();

    // Check if active profile changed, sync bindings if so
    uint8_t current_profile = cfg_mgr.active_profile();
    if (current_profile != g_last_profile) {
        g_last_profile = current_profile;
        sync_tap_dance_bindings(cfg_mgr);
    }

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // 1. Process visual and debounce updates for matrix keys (0..15)
    bool normal_state_changed = false;
    bool normal_consumer_released = false;

    // Expire completed tap-dance pulses
    g_tap_dance_engine.update_pulses(now_ms);

    for (uint8_t i = 0; i < NumMacroKeys; ++i) {
        auto key_id = static_cast<bsp::Key>(i);
        const auto& key_cfg = cfg_mgr.get_active_key_config(i);
        bool is_down = board.keys.is_down(key_id);

        if (board.keys.just_pressed(key_id)) {
            keys[i].was_pressed = true;
            menu.draw_key_slot(board.display, i, true);
            if (key_cfg.binding_type == BindingType::Normal) {
                normal_state_changed = true;
            }
        } else if (board.keys.just_released(key_id)) {
            keys[i].was_pressed = false;
            menu.draw_key_slot(board.display, i, false);
            if (key_cfg.binding_type == BindingType::Normal) {
                normal_state_changed = true;
                if (key_cfg.report_type == ReportType::Consumer) {
                    normal_consumer_released = true;
                }
            }
        }

        // Route TAP_DANCE keys to the tap-dance state buffer
        if (key_cfg.binding_type == BindingType::TapDance) {
            g_tap_dance_engine.update_key(i, now_ms, is_down);
        }
    }

    bool tap_dance_changed = g_tap_dance_engine.has_hid_state_changed();
    if (tap_dance_changed) {
        g_tap_dance_engine.clear_hid_state_changed();
    }

    if (!normal_state_changed && !tap_dance_changed) {
        return;
    }

    if (!tud_hid_ready()) {
        return;
    }

    // 2. Aggregate reports: NORMAL keys (immediate) + TAP_DANCE keys (buffer/pulses/holds)
    uint8_t keycodes[6] = {0};
    uint8_t keycode_idx = 0;
    uint8_t modifiers = 0;
    uint16_t consumer_code = 0;
    bool has_consumer = false;
    bool has_macro = false;

    auto add_keycode = [&](uint8_t code) {
        if (code == 0 || keycode_idx >= 6)
            return;
        for (uint8_t k = 0; k < keycode_idx; ++k) {
            if (keycodes[k] == code)
                return;
        }
        keycodes[keycode_idx++] = code;
    };

    // A. Direct immediate scan for NORMAL keys
    for (uint8_t i = 0; i < NumMacroKeys; ++i) {
        if (board.keys.is_down(static_cast<bsp::Key>(i))) {
            const auto& key_cfg = cfg_mgr.get_active_key_config(i);
            if (key_cfg.binding_type == BindingType::Normal) {
                switch (key_cfg.report_type) {
                case ReportType::Consumer:
                    consumer_code = key_cfg.usage_code;
                    has_consumer = true;
                    break;
                case ReportType::Keyboard: {
                    auto macro = static_cast<MacroCode>(key_cfg.usage_code);
                    switch (macro) {
                    case MacroCode::LockScreenWinLinux: {
                        // Win + L sequence: hold modifier, tap target key, release cleanly
                        has_macro = true;
                        uint8_t win_mod = KEYBOARD_MODIFIER_LEFTGUI;
                        uint8_t l_key[6] = {0x0F, 0, 0, 0, 0, 0}; // 'L'
                        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), win_mod, nullptr);
                        sleep_ms(10);
                        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), win_mod, l_key);
                        sleep_ms(10);
                        uint8_t empty_keys[6] = {0};
                        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), 0, empty_keys);
                        break;
                    }
                    case MacroCode::LockScreenMac: {
                        // Cmd + Ctrl + Q sequence: hold modifiers, tap target key, release cleanly
                        has_macro = true;
                        uint8_t mac_mods = KEYBOARD_MODIFIER_LEFTGUI | KEYBOARD_MODIFIER_LEFTCTRL;
                        uint8_t q_key[6] = {0x14, 0, 0, 0, 0, 0}; // 'Q'
                        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), mac_mods, nullptr);
                        sleep_ms(10);
                        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), mac_mods, q_key);
                        sleep_ms(10);
                        uint8_t empty_keys[6] = {0};
                        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), 0, empty_keys);
                        break;
                    }
                    default:
                        add_keycode(static_cast<uint8_t>(key_cfg.usage_code));
                        break;
                    }
                    break;
                }
                }
            }
        }
    }

    // B. Merge active TAP_DANCE keys (pulses and holds)
    uint8_t td_keycodes[6] = {0};
    uint8_t td_count = 0;
    uint16_t td_consumer = 0;
    bool td_has_consumer = false;
    g_tap_dance_engine.populate_hid_report(td_keycodes, td_count, td_consumer, td_has_consumer);

    for (uint8_t k = 0; k < td_count; ++k) {
        add_keycode(td_keycodes[k]);
    }
    if (td_has_consumer) {
        consumer_code = td_consumer;
        has_consumer = true;
    }

    // 3. Dispatch Consumer Report
    if (has_consumer) {
        tud_hid_report(static_cast<uint8_t>(ReportId::ConsumerControl), &consumer_code, sizeof(consumer_code));
    } else if (normal_consumer_released || (!has_consumer && tap_dance_changed)) {
        uint16_t empty = 0;
        tud_hid_report(static_cast<uint8_t>(ReportId::ConsumerControl), &empty, sizeof(empty));
    }

    // 4. Dispatch Keyboard Report (immediate 0 ms latency for NORMAL keys)
    if (!has_macro && (keycode_idx > 0 || !has_consumer || normal_state_changed || tap_dance_changed)) {
        tud_hid_keyboard_report(static_cast<uint8_t>(ReportId::Keyboard), modifiers, keycodes);
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

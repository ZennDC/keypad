#include "tap_dance.hpp"

namespace zdc::input {

Gesture TapDanceTracker::update(uint32_t now_ms, bool is_down, const TapDanceConfig& cfg) {
    switch (state_) {
    case State::Idle:
        if (is_down) {
            state_ = State::PressDown1;
            timer_start_ms_ = now_ms;
        }
        return Gesture::None;

    case State::PressDown1:
        if (!is_down) {
            // Key released before hold threshold -> candidate for tap/double-tap
            state_ = State::ReleaseUp1;
            timer_start_ms_ = now_ms;
            return Gesture::None;
        } else if (now_ms - timer_start_ms_ >= cfg.hold_timeout_ms) {
            // Key held beyond hold threshold -> Long-press hold triggered
            state_ = State::Holding;
            return Gesture::HoldStart;
        }
        return Gesture::None;

    case State::ReleaseUp1:
        if (is_down) {
            // Pressed a second time within tap window -> second tap candidate
            state_ = State::PressDown2;
            timer_start_ms_ = now_ms;
            return Gesture::None;
        } else if (now_ms - timer_start_ms_ >= cfg.tap_timeout_ms) {
            // Tap window expired without second press -> Single Tap resolved
            state_ = State::Idle;
            return Gesture::SingleTap;
        }
        return Gesture::None;

    case State::PressDown2:
        if (!is_down) {
            // Released on second tap -> Double Tap resolved
            state_ = State::Idle;
            return Gesture::DoubleTap;
        } else if (now_ms - timer_start_ms_ >= cfg.hold_timeout_ms) {
            // Held on second tap -> transition to hold
            state_ = State::Holding;
            return Gesture::HoldStart;
        }
        return Gesture::None;

    case State::Holding:
        if (!is_down) {
            // Held key finally released
            state_ = State::Idle;
            return Gesture::HoldRelease;
        }
        return Gesture::None;
    }

    return Gesture::None;
}

void TapDanceEngine::set_binding(size_t key_idx, const KeyBinding& binding) {
    if (key_idx < KeyCount) {
        bindings_[key_idx] = binding;
    }
}

void TapDanceEngine::set_single_tap(size_t key_idx, Action act) {
    if (key_idx < KeyCount) {
        bindings_[key_idx].single_tap = act;
    }
}

void TapDanceEngine::set_double_tap(size_t key_idx, Action act) {
    if (key_idx < KeyCount) {
        bindings_[key_idx].double_tap = act;
    }
}

void TapDanceEngine::set_hold(size_t key_idx, Action act) {
    if (key_idx < KeyCount) {
        bindings_[key_idx].hold = act;
    }
}

void TapDanceEngine::set_from_key_config(size_t key_idx, const protocol::KeyConfig& cfg) {
    if (key_idx >= KeyCount)
        return;

    KeyBinding b{};
    b.single_tap = Action{cfg.report_type, cfg.usage_code};
    b.hold = Action{cfg.report_type, cfg.usage_code};

    // Smart default for double-tap on media controls
    if (cfg.report_type == protocol::ReportType::Consumer) {
        switch (cfg.usage_code) {
        case 0x00E9: // Volume Increment -> Double-tap = Next Track
            b.double_tap = Action{protocol::ReportType::Consumer, 0x00B5};
            break;
        case 0x00EA: // Volume Decrement -> Double-tap = Previous Track
            b.double_tap = Action{protocol::ReportType::Consumer, 0x00B6};
            break;
        case 0x00CD: // Play / Pause -> Double-tap = Stop
            b.double_tap = Action{protocol::ReportType::Consumer, 0x00B7};
            break;
        default:
            b.double_tap = b.single_tap;
            break;
        }
    } else {
        b.double_tap = b.single_tap;
    }

    bindings_[key_idx] = b;
}

const KeyBinding& TapDanceEngine::binding(size_t key_idx) const {
    static const KeyBinding empty{};
    return (key_idx < KeyCount) ? bindings_[key_idx] : empty;
}

void TapDanceEngine::trigger_gesture(uint8_t key_idx, Gesture gesture, uint32_t now_ms) {
    if (key_idx >= KeyCount)
        return;

    last_gesture_ = gesture;
    last_gesture_key_ = key_idx;

    Action action{};

    switch (gesture) {
    case Gesture::SingleTap:
        action = bindings_[key_idx].single_tap;
        if (action.is_valid()) {
            active_pulses_[key_idx] =
                ActivePulse{.action = action, .expire_time_ms = now_ms + config_.pulse_duration_ms, .active = true};
            hid_state_changed_ = true;
        }
        break;

    case Gesture::DoubleTap:
        action = bindings_[key_idx].double_tap;
        if (action.is_valid()) {
            active_pulses_[key_idx] =
                ActivePulse{.action = action, .expire_time_ms = now_ms + config_.pulse_duration_ms, .active = true};
            hid_state_changed_ = true;
        }
        break;

    case Gesture::HoldStart:
        action = bindings_[key_idx].hold;
        if (action.is_valid()) {
            active_holds_[key_idx] = action;
            hid_state_changed_ = true;
        }
        break;

    case Gesture::HoldRelease:
        action = bindings_[key_idx].hold;
        if (active_holds_[key_idx].is_valid()) {
            active_holds_[key_idx] = {};
            hid_state_changed_ = true;
        }
        break;

    default:
        break;
    }

    if (gesture_callback_) {
        gesture_callback_(key_idx, gesture, action);
    }
}

void TapDanceEngine::update_pulses(uint32_t now_ms) {
    for (size_t i = 0; i < KeyCount; ++i) {
        if (active_pulses_[i].active && now_ms >= active_pulses_[i].expire_time_ms) {
            active_pulses_[i].active = false;
            hid_state_changed_ = true;
        }
    }
}

void TapDanceEngine::update_key(size_t key_idx, uint32_t now_ms, bool is_down) {
    if (key_idx >= KeyCount)
        return;

    Gesture g = trackers_[key_idx].update(now_ms, is_down, config_);
    if (g != Gesture::None) {
        trigger_gesture(static_cast<uint8_t>(key_idx), g, now_ms);
    }
}

void TapDanceEngine::reset_key(size_t key_idx) {
    if (key_idx >= KeyCount)
        return;

    trackers_[key_idx].reset();
    if (active_holds_[key_idx].is_valid()) {
        active_holds_[key_idx] = {};
        hid_state_changed_ = true;
    }
    if (active_pulses_[key_idx].active) {
        active_pulses_[key_idx].active = false;
        hid_state_changed_ = true;
    }
}

void TapDanceEngine::update(uint32_t now_ms, const bsp::Keyboard& keyboard) {
    // 1. Expire completed tap pulses
    update_pulses(now_ms);

    // 2. Advance state machines for all physical keys
    for (size_t i = 0; i < KeyCount; ++i) {
        auto key = static_cast<bsp::Key>(i);
        update_key(i, now_ms, keyboard.is_down(key));
    }
}

void TapDanceEngine::populate_hid_report(uint8_t keycodes[6], uint8_t& keycode_count, uint16_t& consumer_code,
                                         bool& has_consumer) {
    keycode_count = 0;
    consumer_code = 0;
    has_consumer = false;

    auto add_action = [&](const Action& act) {
        if (!act.is_valid())
            return;

        if (act.report_type == protocol::ReportType::Consumer) {
            consumer_code = act.usage_code;
            has_consumer = true;
        } else if (act.report_type == protocol::ReportType::Keyboard) {
            if (keycode_count < 6) {
                // Deduplicate keycodes
                for (size_t k = 0; k < keycode_count; ++k) {
                    if (keycodes[k] == static_cast<uint8_t>(act.usage_code))
                        return;
                }
                keycodes[keycode_count++] = static_cast<uint8_t>(act.usage_code);
            }
        }
    };

    // Add continuous held actions
    for (size_t i = 0; i < KeyCount; ++i) {
        if (active_holds_[i].is_valid()) {
            add_action(active_holds_[i]);
        }
    }

    // Add momentary tap pulses
    for (size_t i = 0; i < KeyCount; ++i) {
        if (active_pulses_[i].active) {
            add_action(active_pulses_[i].action);
        }
    }
}

bool TapDanceEngine::is_key_holding(size_t key_idx) const noexcept {
    return (key_idx < KeyCount) && trackers_[key_idx].is_holding();
}

TapDanceTracker::State TapDanceEngine::key_state(size_t key_idx) const noexcept {
    return (key_idx < KeyCount) ? trackers_[key_idx].state() : TapDanceTracker::State::Idle;
}

} // namespace zdc::input

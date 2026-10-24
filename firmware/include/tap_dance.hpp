#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#include "bsp/keyboard.hpp"
#include "protocol.hpp"

namespace zdc::input {

// ---------------------------------------------------------------------------
// Gestures / Tap-Dance Actions
// ---------------------------------------------------------------------------
enum class Gesture : uint8_t {
    None = 0,
    SingleTap,  // Pressed and released once; tap window expired without second tap
    DoubleTap,  // Pressed, released, and pressed+released a second time within tap window
    HoldStart,  // Key held down continuously beyond hold threshold
    HoldRelease // Key in hold state was released
};

[[nodiscard]] constexpr const char* to_string(Gesture g) noexcept {
    switch (g) {
    case Gesture::SingleTap:
        return "Single Tap";
    case Gesture::DoubleTap:
        return "Double Tap";
    case Gesture::HoldStart:
        return "Hold";
    case Gesture::HoldRelease:
        return "Hold Release";
    default:
        return "None";
    }
}

// ---------------------------------------------------------------------------
// Action Definitions
// ---------------------------------------------------------------------------
struct Action {
    protocol::ReportType report_type{protocol::ReportType::Keyboard};
    uint16_t usage_code{0};

    [[nodiscard]] constexpr bool is_valid() const noexcept { return usage_code != 0; }

    [[nodiscard]] constexpr bool operator==(const Action& other) const noexcept {
        return report_type == other.report_type && usage_code == other.usage_code;
    }
};

struct KeyBinding {
    Action single_tap{};
    Action double_tap{};
    Action hold{};
};

// ---------------------------------------------------------------------------
// Timing Configuration
// ---------------------------------------------------------------------------
struct TapDanceConfig {
    uint32_t tap_timeout_ms{200};   // Max delay between 1st release and 2nd press (ms)
    uint32_t hold_timeout_ms{300};  // Min hold time to trigger long-press hold (ms)
    uint32_t pulse_duration_ms{25}; // Duration of tap pulse for USB HID delivery (ms)
};

// ---------------------------------------------------------------------------
// Per-Key State Machine Tracker
// ---------------------------------------------------------------------------
class TapDanceTracker {
public:
    enum class State : uint8_t {
        Idle,       // Key is released, no pending gestures
        PressDown1, // Key pressed down (1st tap), awaiting release or hold timeout
        ReleaseUp1, // Key released (1st tap), awaiting 2nd tap or tap timeout
        PressDown2, // Key pressed down (2nd tap), awaiting release (double tap) or hold
        Holding     // Key is currently in continuous hold
    };

    TapDanceTracker() = default;

    // Advances the state machine by inspecting current physical key state and time
    Gesture update(uint32_t now_ms, bool is_down, const TapDanceConfig& cfg);

    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] bool is_holding() const noexcept { return state_ == State::Holding; }
    [[nodiscard]] bool is_active() const noexcept { return state_ != State::Idle; }

    void reset() noexcept {
        state_ = State::Idle;
        timer_start_ms_ = 0;
    }

private:
    State state_{State::Idle};
    uint32_t timer_start_ms_{0};
};

// ---------------------------------------------------------------------------
// Tap-Dance & Hold Engine
// ---------------------------------------------------------------------------
class TapDanceEngine {
public:
    static constexpr size_t KeyCount = static_cast<size_t>(bsp::Key::Count); // 16 matrix + 2 side buttons

    using GestureCallback = std::function<void(uint8_t key_index, Gesture gesture, const Action& action)>;

    explicit TapDanceEngine(TapDanceConfig config = {}) : config_(config) {}

    // Configuration
    void set_config(const TapDanceConfig& cfg) noexcept { config_ = cfg; }
    [[nodiscard]] const TapDanceConfig& config() const noexcept { return config_; }

    void set_binding(size_t key_idx, const KeyBinding& binding);
    void set_single_tap(size_t key_idx, Action act);
    void set_double_tap(size_t key_idx, Action act);
    void set_hold(size_t key_idx, Action act);

    // Initialize binding for a key from standard persistent KeyConfig
    void set_from_key_config(size_t key_idx, const protocol::KeyConfig& cfg);

    [[nodiscard]] const KeyBinding& binding(size_t key_idx) const;

    // Callback notification for detected gestures
    void set_gesture_callback(GestureCallback cb) { gesture_callback_ = std::move(cb); }

    // Update routine called in main loop
    void update(uint32_t now_ms, const bsp::Keyboard& keyboard);

    // HID report generator: populates active keyboard keycodes & consumer control
    void populate_hid_report(uint8_t keycodes[6], uint8_t& keycode_count, uint16_t& consumer_code, bool& has_consumer);

    // Returns whether HID report state changed and needs transmission
    [[nodiscard]] bool has_hid_state_changed() const noexcept { return hid_state_changed_; }
    void clear_hid_state_changed() noexcept { hid_state_changed_ = false; }

    // State query
    [[nodiscard]] bool is_key_holding(size_t key_idx) const noexcept;
    [[nodiscard]] TapDanceTracker::State key_state(size_t key_idx) const noexcept;

    // Most recent detected gesture for UI display
    [[nodiscard]] Gesture last_gesture() const noexcept { return last_gesture_; }
    [[nodiscard]] uint8_t last_gesture_key() const noexcept { return last_gesture_key_; }

private:
    struct ActivePulse {
        Action action{};
        uint32_t expire_time_ms{0};
        bool active{false};
    };

    TapDanceConfig config_{};
    std::array<TapDanceTracker, KeyCount> trackers_{};
    std::array<KeyBinding, KeyCount> bindings_{};

    // Active held actions (one slot per key)
    std::array<Action, KeyCount> active_holds_{};

    // Active tap pulses (temporary key presses)
    std::array<ActivePulse, KeyCount> active_pulses_{};

    bool hid_state_changed_{false};
    GestureCallback gesture_callback_{nullptr};

    Gesture last_gesture_{Gesture::None};
    uint8_t last_gesture_key_{0};

    void trigger_gesture(uint8_t key_idx, Gesture gesture, uint32_t now_ms);
};

} // namespace zdc::input

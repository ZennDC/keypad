#include "tap_dance.hpp"
#include <cassert>
#include <cstdio>
#include <vector>

// Mock for host testing
static uint32_t mock_keys_down = 0;
namespace bsp {
bool Keyboard::is_down(Key key) const {
    return (mock_keys_down & (1UL << static_cast<uint8_t>(key))) != 0;
}
} // namespace bsp

using namespace zdc::input;
using namespace zdc::protocol;

void test_tracker_single_tap() {
    TapDanceTracker tracker;
    TapDanceConfig cfg{.tap_timeout_ms = 200, .hold_timeout_ms = 300};

    // t=100: Press key
    assert(tracker.update(100, true, cfg) == Gesture::None);
    assert(tracker.state() == TapDanceTracker::State::PressDown1);

    // t=150: Still held (50ms)
    assert(tracker.update(150, true, cfg) == Gesture::None);

    // t=180: Released (held for 80ms < 300ms)
    assert(tracker.update(180, false, cfg) == Gesture::None);
    assert(tracker.state() == TapDanceTracker::State::ReleaseUp1);

    // t=300: 120ms after release (< 200ms)
    assert(tracker.update(300, false, cfg) == Gesture::None);

    // t=380: 200ms after release -> SingleTap resolved!
    assert(tracker.update(380, false, cfg) == Gesture::SingleTap);
    assert(tracker.state() == TapDanceTracker::State::Idle);

    printf("PASS: test_tracker_single_tap\n");
}

void test_tracker_double_tap() {
    TapDanceTracker tracker;
    TapDanceConfig cfg{.tap_timeout_ms = 200, .hold_timeout_ms = 300};

    // 1st Tap: Press at t=100, release at t=150
    assert(tracker.update(100, true, cfg) == Gesture::None);
    assert(tracker.update(150, false, cfg) == Gesture::None);
    assert(tracker.state() == TapDanceTracker::State::ReleaseUp1);

    // 2nd Tap: Press at t=250 (100ms after release, < 200ms)
    assert(tracker.update(250, true, cfg) == Gesture::None);
    assert(tracker.state() == TapDanceTracker::State::PressDown2);

    // Release 2nd tap at t=310
    assert(tracker.update(310, false, cfg) == Gesture::DoubleTap);
    assert(tracker.state() == TapDanceTracker::State::Idle);

    printf("PASS: test_tracker_double_tap\n");
}

void test_tracker_long_press_hold() {
    TapDanceTracker tracker;
    TapDanceConfig cfg{.tap_timeout_ms = 200, .hold_timeout_ms = 300};

    // Press at t=100
    assert(tracker.update(100, true, cfg) == Gesture::None);

    // t=250 (150ms held)
    assert(tracker.update(250, true, cfg) == Gesture::None);

    // t=400 (300ms held -> HoldStart!)
    assert(tracker.update(400, true, cfg) == Gesture::HoldStart);
    assert(tracker.state() == TapDanceTracker::State::Holding);
    assert(tracker.is_holding());

    // t=600: Still holding
    assert(tracker.update(600, true, cfg) == Gesture::None);
    assert(tracker.is_holding());

    // t=800: Release -> HoldRelease!
    assert(tracker.update(800, false, cfg) == Gesture::HoldRelease);
    assert(tracker.state() == TapDanceTracker::State::Idle);
    assert(!tracker.is_holding());

    printf("PASS: test_tracker_long_press_hold\n");
}

void test_tracker_second_press_hold() {
    TapDanceTracker tracker;
    TapDanceConfig cfg{.tap_timeout_ms = 200, .hold_timeout_ms = 300};

    // 1st Tap: Press at t=100, release at t=150
    assert(tracker.update(100, true, cfg) == Gesture::None);
    assert(tracker.update(150, false, cfg) == Gesture::None);

    // 2nd Tap: Press at t=200
    assert(tracker.update(200, true, cfg) == Gesture::None);
    assert(tracker.state() == TapDanceTracker::State::PressDown2);

    // Hold 2nd tap past hold_timeout (t=500 -> 300ms since 2nd press)
    assert(tracker.update(500, true, cfg) == Gesture::HoldStart);
    assert(tracker.state() == TapDanceTracker::State::Holding);

    // Release at t=700
    assert(tracker.update(700, false, cfg) == Gesture::HoldRelease);
    assert(tracker.state() == TapDanceTracker::State::Idle);

    printf("PASS: test_tracker_second_press_hold\n");
}

void test_engine_hid_generation() {
    TapDanceEngine engine;
    bsp::Keyboard kbd;

    // Configure key 0: Single Tap = 'A' (0x04), Double Tap = 'B' (0x05), Hold = 'C' (0x06)
    engine.set_binding(0, KeyBinding{.single_tap = Action{ReportType::Keyboard, 0x04},
                                     .double_tap = Action{ReportType::Keyboard, 0x05},
                                     .hold = Action{ReportType::Keyboard, 0x06}});

    uint8_t keycodes[6] = {0};
    uint8_t count = 0;
    uint16_t consumer = 0;
    bool has_consumer = false;

    // Initially idle
    mock_keys_down = 0;
    engine.update(0, kbd);
    engine.populate_hid_report(keycodes, count, consumer, has_consumer);
    assert(count == 0);
    assert(!has_consumer);

    // 1. Test Hold on Key 0
    mock_keys_down = (1 << 0);
    engine.update(100, kbd);
    engine.populate_hid_report(keycodes, count, consumer, has_consumer);
    assert(count == 0); // Not held yet

    // At t=400 (300ms held): Hold starts!
    engine.update(400, kbd);
    engine.populate_hid_report(keycodes, count, consumer, has_consumer);
    assert(count == 1);
    assert(keycodes[0] == 0x06); // 'C' is held!

    // Release at t=600
    mock_keys_down = 0;
    engine.update(600, kbd);
    engine.populate_hid_report(keycodes, count, consumer, has_consumer);
    assert(count == 0); // Released!

    // 2. Test Double Tap on Key 0
    // Tap 1: press at 1000, release at 1050
    mock_keys_down = (1 << 0);
    engine.update(1000, kbd);
    mock_keys_down = 0;
    engine.update(1050, kbd);

    // Tap 2: press at 1150, release at 1200
    mock_keys_down = (1 << 0);
    engine.update(1150, kbd);
    mock_keys_down = 0;
    engine.update(1200, kbd);

    // Double tap pulse active!
    engine.populate_hid_report(keycodes, count, consumer, has_consumer);
    assert(count == 1);
    assert(keycodes[0] == 0x05); // 'B' (double tap action)

    // After pulse duration (t=1230 > 1200 + 25): pulse expires!
    engine.update(1230, kbd);
    engine.populate_hid_report(keycodes, count, consumer, has_consumer);
    assert(count == 0); // Pulse expired!

    printf("PASS: test_engine_hid_generation\n");
}

int main() {
    test_tracker_single_tap();
    test_tracker_double_tap();
    test_tracker_long_press_hold();
    test_tracker_second_press_hold();
    test_engine_hid_generation();
    printf("\nALL TAP-DANCE & HOLD UNIT TESTS PASSED!\n");
    return 0;
}

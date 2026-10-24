# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Per-Key Tap-Dance Configuration**:
  - Added `BindingType` (`Normal = 0x00`, `TapDance = 0x01`) enum to shared protocol definition.
  - Extended `KeyConfig` data structure with `binding_type` field.
  - Added per-key state machine dispatch and reset support in `TapDanceEngine` (`update_key`, `update_pulses`, `reset_key`).
  - Added automatic backwards compatibility migration in `FlashStorage::read()` from Version 1 flash images to Version 2 with default `NORMAL` bindings.
  - Added Catch2 unit tests validating protocol packing, alignment, checksum invalidation on `binding_type` changes, and legacy V1-to-V2 automatic upgrades.
- **Companion App & Host Protocol Documentation**:
  - Documented binary payload format for USB Feature Report 3 (`SET_REPORT` byte 4: `binding_type`).
  - Defined JSON Schema for key mapping configuration.

### Changed
- **Matrix Scanning & HID Event Routing**:
  - Keys configured as `NORMAL` bypass the tap-dance state buffer and are dispatched immediately via `tud_hid_keyboard_report()` and `tud_hid_report()` with 0 ms latency.
  - Keys configured as `TAP_DANCE` are routed to the timing buffer and tap-dance state machine (single-tap, double-tap, and hold gestures).
  - Main event loop aggregates active `NORMAL` keys with active `TAP_DANCE` pulses and hold states before USB report transmission.
- **Flash Storage & Configuration Versioning**:
  - Incremented `FlashConfigVersion` from `1` to `2`.
  - Updated default configurations and presets (`NumpadPreset`, `MediaPreset`, `NavPreset`) to default to `BindingType::Normal`.
  - Updated `ConfigManager::handle_packet` to parse optional 5th byte in single-key feature reports for `binding_type`.
- **Firmware Build System**:
  - Included `src/tap_dance.cpp` in `Keypad` executable sources in `firmware/CMakeLists.txt`.

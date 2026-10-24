# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Side Buttons & On-Screen Settings Menu**:
  - Implemented release-based gesture engine for side buttons (`SideBtn1` on GP28 = Up/Previous profile, `SideBtn2` on GP29 = Down/Next profile, dual-button press = Select / Enter / Open Settings Menu).
  - Added Settings Menu options: display brightness adjustment (20%–100%), sleep timeout selection (Off, 1m, 5m, 15m), direct BOOTSEL / UF2 reboot, and Save & Exit to Flash.
  - Added inactivity auto-exit (8s timeout) that commits pending brightness adjustments to persistent Flash.
- **Hardware Recovery & Bootloader Failsafes**:
  - Cold-boot failsafe: Holding `SideBtn1` while plugging in USB power immediately jumps to RP2040 BOOTSEL mode.
  - Runtime BOOTSEL countdown: Holding `SideBtn1` for 3.5s displays an on-screen countdown and progress bar before jumping to BOOTSEL.
  - Factory reset countdown: Holding both side buttons for 5.0s displays an on-screen countdown and progress bar before wiping the Flash sector and rebooting with factory defaults.
  - Clean cancelation: Releasing buttons before countdown completion safely aborts the operation and returns to the normal UI.
- **Composite System Macros**:
  - Added multi-key sequence dispatcher in firmware event loop for system macros.
  - Added OS lock screen macros (`Lock Screen (Win/Linux)` via Win+L, `Lock Screen (macOS)` via Cmd+Ctrl+Q).
  - Extended shared protocol with `MacroCode` and standard `KeyCode` definitions.
- **Configurator Actions Catalog**:
  - Added Lock Screen actions with lock icons to the host action catalog.
  - Added navigation arrow keys (Left, Right, Up, Down) and Print Screen to available keyboard bindings.
- **Host USB Status Indicators**:
  - Added real-time bidirectional USB HID LED status badges (`[NUM]`, `[CAP]`, `[SCR]`) driven by OS keyboard output reports.
- **CI/CD & Native Host Test Harness**:
  - Added native Catch2 CTest test suite under `tests/` for continuous protocol struct, checksum, and migration verification on host OS.
  - Added RP2040 Flash (2 MB) and SRAM (264 KB) footprint check script (`firmware/tools/check_firmware_size.py`).
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

- **Display Rendering & Performance**:
  - ST7789 display initialized strictly in fixed hardware 90° landscape mode (`madctl = 0x70`, 320×240); dynamic rotation options removed.
  - Implemented differential row rendering in Settings Menu to eliminate full-screen wipe flickers over SPI.
  - Converted countdown dialogs to draw frame once and differentially stream only modified seconds text and forward-advancing progress bar pixels.
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

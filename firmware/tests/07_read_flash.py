#!/usr/bin/env python3
"""
07_read_flash.py

Reads and decodes the new version (v3) of persistent flash memory / configuration
from the ZDC MacroPad.

Supports:
1. Live USB HID interrogation:
   - Reads global display settings (mode, orientation, brightness).
   - Reads profile metadata (active profile, reported profile, total profile count).
   - Queries all profiles (0 to profile_count - 1) via Command::SelectProfile (0xA1).
   - Decodes per-key binding types (Normal vs TapDance) from the 16-bit tap-dance bitmask.
   - Decodes key usage codes (Standard HID Keyboard, Consumer Media, and System Macros).
   - Decodes profile names.
2. Raw flash dump / image file decoding (e.g. 336-byte PersistentConfig or 4KB flash sector dump):
   - Validates Magic (0x5A444331 / 'ZDC1').
   - Validates FlashConfigVersion (v3).
   - Verifies 8-bit checksum against memory bytes.
   - Unpacks all ProfileConfig entries directly.
"""

import argparse
import struct
import sys
import time
from typing import Optional

# ---------------------------------------------------------------------------
# Protocol Constants (shared/include/protocol.hpp)
# ---------------------------------------------------------------------------
VID = 0xCAFE
PID = 0x4000
REPORT_ID = 3
TOTAL_PACKET_LEN = 64
DATA_LEN = TOTAL_PACKET_LEN - 1  # 63 payload bytes

FLASH_MAGIC = 0x5A444331  # ASCII "ZDC1"
FLASH_VERSION_V3 = 3
MAX_PROFILES = 4
NUM_KEYS = 16
PROFILE_NAME_LEN = 16
PERSISTENT_CONFIG_SIZE = 336  # Struct aligned to 4 bytes

CMD_SELECT_PROFILE = 0xA1

REPORT_TYPES = {
    0: "Disabled",
    1: "Keyboard",
    2: "Consumer",
}

BINDING_TYPES = {
    0: "Normal",
    1: "TapDance",
}

DISPLAY_MODES = {
    0: "HwMonitor",
    1: "Profile",
    2: "Image",
}

# Known HID Keyboard Usage Codes
KEY_NAMES = {
    0x00: "None",
    0x04: "A",
    0x05: "B",
    0x06: "C",
    0x07: "D",
    0x08: "E",
    0x09: "F",
    0x0A: "G",
    0x0B: "H",
    0x0C: "I",
    0x0D: "J",
    0x0E: "K",
    0x0F: "L",
    0x10: "M",
    0x11: "N",
    0x12: "O",
    0x13: "P",
    0x14: "Q",
    0x15: "R",
    0x16: "S",
    0x17: "T",
    0x18: "U",
    0x19: "V",
    0x1A: "W",
    0x1B: "X",
    0x1C: "Y",
    0x1D: "Z",
    0x1E: "1",
    0x1F: "2",
    0x20: "3",
    0x21: "4",
    0x22: "5",
    0x23: "6",
    0x24: "7",
    0x25: "8",
    0x26: "9",
    0x27: "0",
    0x28: "Enter",
    0x29: "Escape",
    0x2A: "Backspace",
    0x2B: "Tab",
    0x2C: "Space",
    0x46: "PrintScreen",
    0x4A: "Home",
    0x4B: "PageUp",
    0x4C: "Delete",
    0x4D: "End",
    0x4E: "PageDown",
    0x4F: "RightArrow",
    0x50: "LeftArrow",
    0x51: "DownArrow",
    0x52: "UpArrow",
    0x54: "KP /",
    0x55: "KP *",
    0x56: "KP -",
    0x57: "KP +",
    0x58: "KP Enter",
    0x59: "KP 1",
    0x5A: "KP 2",
    0x5B: "KP 3",
    0x5C: "KP 4",
    0x5D: "KP 5",
    0x5E: "KP 6",
    0x5F: "KP 7",
    0x60: "KP 8",
    0x61: "KP 9",
    0x62: "KP 0",
    0x63: "KP .",
    0x68: "F13",
    0x69: "F14",
    0x6A: "F15",
    0x6B: "F16",
}

# Known HID Consumer Media Usage Codes
CONSUMER_NAMES = {
    0x0032: "Sleep",
    0x006F: "Brightness Up",
    0x0070: "Brightness Down",
    0x00B3: "Fast Forward",
    0x00B4: "Rewind",
    0x00B5: "Next Track",
    0x00B6: "Previous Track",
    0x00B7: "Stop",
    0x00B8: "Eject",
    0x00CD: "Play / Pause",
    0x00E2: "Mute",
    0x00E9: "Volume Up",
    0x00EA: "Volume Down",
    0x0192: "Calculator",
    0x0194: "File Browser",
    0x0223: "Home",
}

# Known System Macro Codes
MACRO_NAMES = {
    0x1001: "Macro: LockScreen (Win/Linux)",
    0x1002: "Macro: LockScreen (Mac)",
}

DEFAULT_PROFILE_NAMES = {
    0: "Custom",
    1: "Numpad",
    2: "Media",
    3: "Navigation",
}


def describe_usage(report_type: int, usage_code: int) -> str:
    """Returns a readable name for a key usage code."""
    if usage_code in MACRO_NAMES:
        return MACRO_NAMES[usage_code]

    if report_type == 1:  # Keyboard
        return KEY_NAMES.get(usage_code, f"Key 0x{usage_code:02X}")
    elif report_type == 2:  # Consumer
        return CONSUMER_NAMES.get(usage_code, f"Consumer 0x{usage_code:04X}")
    elif report_type == 0:
        return "None"
    return f"Usage 0x{usage_code:04X}"


# ---------------------------------------------------------------------------
# HID Device Interrogation
# ---------------------------------------------------------------------------
def read_via_hid():
    try:
        import hid
    except ImportError:
        print("Error: 'hid' (hidapi) Python package not installed.")
        print("Install it with: pip install hidapi")
        sys.exit(1)

    device = hid.device()
    try:
        print(f"Connecting to ZDC MacroPad (VID=0x{VID:04X}, PID=0x{PID:04X})...")
        device.open(VID, PID)
    except OSError as err:
        print(f"Error: Unable to open device: {err}")
        print(
            "Ensure the device is connected and permissions allow access (udev rules)."
        )
        sys.exit(1)

    try:
        prod = device.get_product_string() or "ZDC MacroPad"
        mfg = device.get_manufacturer_string() or "ZDC Engineering"
        serial = device.get_serial_number_string() or "N/A"
        print(f"Connected: {prod} by {mfg} (Serial: {serial})\n")

        # Read initial feature report
        raw_initial = device.get_feature_report(REPORT_ID, TOTAL_PACKET_LEN)
        if not raw_initial:
            raise RuntimeError("Failed to read feature report from device.")

        data_initial = raw_initial[1:] if raw_initial[0] == REPORT_ID else raw_initial

        # Display settings (bytes 48 - 50)
        disp_mode = data_initial[48]
        disp_orient = data_initial[49]
        disp_bright = data_initial[50]

        orient_map = {0: "0°", 1: "90°", 2: "180°", 3: "270°"}
        print(
            "╔════════════════════════════════════════════════════════════════════════════╗"
        )
        print(
            "║                     GLOBAL DISPLAY SETTINGS (FLASH v3)                     ║"
        )
        print(
            "╠════════════════════════════════════════════════════════════════════════════╣"
        )
        print(
            f"║ Display Mode:        {DISPLAY_MODES.get(disp_mode, f'Unknown ({disp_mode})'):<20} (Raw: {disp_mode})          ║"
        )
        print(
            f"║ Orientation:         {orient_map.get(disp_orient, f'{disp_orient}'):<20} (Raw: {disp_orient})          ║"
        )
        print(
            f"║ Brightness:          {disp_bright}%                                              ║"
        )
        print(
            "╚════════════════════════════════════════════════════════════════════════════╝\n"
        )

        # Profile metadata (byte 53)
        meta = data_initial[53]
        initial_reported_prof = meta & 0x03
        active_prof = (meta >> 2) & 0x03
        total_profiles = (meta >> 4) & 0x0F
        if total_profiles == 0 or total_profiles > MAX_PROFILES:
            total_profiles = MAX_PROFILES

        print(
            f"Profile Status: Active Profile = [{active_prof}], Total Profiles in Flash = {total_profiles}\n"
        )

        # Query and display each profile
        for prof_idx in range(total_profiles):
            if prof_idx == initial_reported_prof:
                data = data_initial
            else:
                # Query profile without activating it: Command 0xA1, byte 1 = profile, byte 2 = 0 (activate=false)
                padded_cmd = [REPORT_ID, CMD_SELECT_PROFILE, prof_idx, 0] + [0x00] * (
                    DATA_LEN - 3
                )
                device.send_feature_report(padded_cmd)
                time.sleep(0.015)

                raw = device.get_feature_report(REPORT_ID, TOTAL_PACKET_LEN)
                if not raw:
                    print(f"Warning: Failed to fetch report for profile {prof_idx}")
                    continue
                data = raw[1:] if raw[0] == REPORT_ID else raw

            # Parse tap-dance mask (bytes 51 - 52)
            td_mask = data[51] | (data[52] << 8)

            # Parse profile name (bytes 54 - 63)
            name_bytes = bytes(data[54:64]).split(b"\x00")[0]
            try:
                prof_name = name_bytes.decode("utf-8").strip()
            except UnicodeDecodeError:
                prof_name = ""
            if not prof_name:
                prof_name = DEFAULT_PROFILE_NAMES.get(
                    prof_idx, f"Profile {prof_idx + 1}"
                )

            active_tag = " [ACTIVE]" if prof_idx == active_prof else ""
            print(
                f"┌────────────────────────────────────────────────────────────────────────────┐"
            )
            print(
                f"│ PROFILE {prof_idx}: {prof_name:<20}{active_tag:<15} (TD Mask: 0x{td_mask:04X})   │"
            )
            print(
                f"├─────┬───────────┬──────────────┬─────────────┬─────────────────────────────┤"
            )
            print(
                f"│ Key │ Type      │ Binding      │ Usage Code  │ Name / Description          │"
            )
            print(
                f"├─────┼───────────┼──────────────┼─────────────┼─────────────────────────────┤"
            )

            offset = 0
            for k in range(NUM_KEYS):
                rpt_type = data[offset]
                usage = data[offset + 1] | (data[offset + 2] << 8)
                offset += 3

                is_td = bool(td_mask & (1 << k))
                binding_str = "TapDance" if is_td else "Normal"
                rpt_str = REPORT_TYPES.get(rpt_type, f"Raw({rpt_type})")
                desc = describe_usage(rpt_type, usage)

                print(
                    f"│ {k:2d}  │ {rpt_str:<9} │ {binding_str:<12} │ 0x{usage:04X}     │ {desc:<27} │"
                )

            print(
                f"└─────┴───────────┴──────────────┴─────────────┴─────────────────────────────┘\n"
            )

        # Restore target query profile back to active_profile
        padded_restore = [REPORT_ID, CMD_SELECT_PROFILE, active_prof, 0] + [0x00] * (
            DATA_LEN - 3
        )
        device.send_feature_report(padded_restore)

    finally:
        device.close()
        print("Device connection closed successfully.")


# ---------------------------------------------------------------------------
# Offline Binary Flash Dump Decoder
# ---------------------------------------------------------------------------
def decode_flash_binary(data: bytes):
    """
    Decodes a PersistentConfig v3 binary structure (336 bytes).
    Layout:
      0..3:   uint32 magic (0x5A444331)
      4:      uint8 version (3)
      5..7:   DisplayConfig (mode, orientation, brightness)
      8:      uint8 profile_count
      9:      uint8 active_profile
      10..11: uint8 reserved[2]
      12..331: 4 * ProfileConfig (80 bytes each)
      332:    uint8 checksum
      333..335: padding
    """
    if len(data) < PERSISTENT_CONFIG_SIZE:
        # Check if this might be a 4KB sector dump where config is located at offset 0
        raise ValueError(
            f"Data size ({len(data)} bytes) is smaller than PersistentConfig ({PERSISTENT_CONFIG_SIZE} bytes)."
        )

    magic, version, disp_mode, disp_orient, disp_bright, prof_count, active_prof = (
        struct.unpack_from("<IBBBBBB", data, 0)
    )

    print(
        "╔════════════════════════════════════════════════════════════════════════════╗"
    )
    print(
        "║                 PERSISTENT FLASH MEMORY BINARY IMAGE (v3)                  ║"
    )
    print(
        "╠════════════════════════════════════════════════════════════════════════════╣"
    )
    magic_ascii = "".join(chr((magic >> (8 * i)) & 0xFF) for i in range(4))
    print(
        f"║ Magic:               0x{magic:08X} ('{magic_ascii}')                                    ║"
    )
    print(
        f"║ Flash Version:       {version}                                                     ║"
    )
    print(
        f"║ Profile Count:       {prof_count} (Max: {MAX_PROFILES})                                           ║"
    )
    print(
        f"║ Active Profile:      {active_prof}                                                     ║"
    )
    print(
        f"║ Display Mode:        {DISPLAY_MODES.get(disp_mode, f'Unknown ({disp_mode})'):<20} (Raw: {disp_mode})          ║"
    )
    print(
        f"║ Display Orientation: {disp_orient}                                                     ║"
    )
    print(
        f"║ Display Brightness:  {disp_bright}%                                                    ║"
    )

    # Checksum validation: sum of bytes 0..331 modulo 256
    expected_chk = sum(data[:332]) & 0xFF
    actual_chk = data[332]
    chk_status = (
        "VALID (OK)"
        if expected_chk == actual_chk
        else f"INVALID (computed 0x{expected_chk:02X})"
    )
    print(f"║ Checksum:            0x{actual_chk:02X} -> {chk_status:<36}║")
    print(
        "╚════════════════════════════════════════════════════════════════════════════╝\n"
    )

    if magic != FLASH_MAGIC:
        print(
            f"WARNING: Magic mismatch! Expected 0x{FLASH_MAGIC:08X}, got 0x{magic:08X}"
        )
    if version != FLASH_VERSION_V3:
        print(f"WARNING: Version mismatch! Expected {FLASH_VERSION_V3}, got {version}")

    profiles_offset = 12
    for p in range(min(prof_count, MAX_PROFILES)):
        p_base = profiles_offset + (p * 80)
        name_raw = data[p_base : p_base + PROFILE_NAME_LEN]
        name = name_raw.split(b"\x00")[0].decode("utf-8", errors="replace")
        if not name:
            name = DEFAULT_PROFILE_NAMES.get(p, f"Profile {p + 1}")

        active_tag = " [ACTIVE]" if p == active_prof else ""
        print(
            f"┌────────────────────────────────────────────────────────────────────────────┐"
        )
        print(
            f"│ PROFILE {p}: {name:<20}{active_tag:<15}                             │"
        )
        print(
            f"├─────┬───────────┬──────────────┬─────────────┬─────────────────────────────┤"
        )
        print(
            f"│ Key │ Type      │ Binding      │ Usage Code  │ Name / Description          │"
        )
        print(
            f"├─────┼───────────┼──────────────┼─────────────┼─────────────────────────────┤"
        )

        keys_base = p_base + PROFILE_NAME_LEN
        for k in range(NUM_KEYS):
            k_off = keys_base + (k * 4)
            rpt_type, binding_type, usage = struct.unpack_from("<BBH", data, k_off)

            rpt_str = REPORT_TYPES.get(rpt_type, f"Raw({rpt_type})")
            bind_str = BINDING_TYPES.get(binding_type, f"Raw({binding_type})")
            desc = describe_usage(rpt_type, usage)

            print(
                f"│ {k:2d}  │ {rpt_str:<9} │ {bind_str:<12} │ 0x{usage:04X}     │ {desc:<27} │"
            )

        print(
            f"└─────┴───────────┴──────────────┴─────────────┴─────────────────────────────┘\n"
        )


# ---------------------------------------------------------------------------
# Main Entry Point
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Read and decode the new version (v3) of ZDC MacroPad flash memory / configuration."
    )
    parser.add_argument(
        "-f",
        "--file",
        type=str,
        help="Path to a raw binary flash dump or PersistentConfig image file to decode offline.",
    )
    args = parser.parse_args()

    if args.file:
        try:
            with open(args.file, "rb") as fp:
                file_data = fp.read()
            print(f"Loaded {len(file_data)} bytes from '{args.file}'.\n")
            # If a full 2MB or 4KB flash dump was passed, scan for the magic word
            if len(file_data) > PERSISTENT_CONFIG_SIZE:
                idx = file_data.find(b"ZDC1")
                if idx != -1:
                    print(
                        f"Found 'ZDC1' magic at offset 0x{idx:X} ({idx}). Decoding struct from there...\n"
                    )
                    file_data = file_data[idx:]
            decode_flash_binary(file_data)
        except Exception as err:
            print(f"Error reading file '{args.file}': {err}")
            sys.exit(1)
    else:
        read_via_hid()


if __name__ == "__main__":
    main()

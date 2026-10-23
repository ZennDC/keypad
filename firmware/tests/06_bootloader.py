#!/usr/bin/env python3
"""
Sends the bootloader command (0xBB) to the ZDC MacroPad to reboot it into UF2 / BOOTSEL mode.
"""

import sys
from contextlib import suppress

import hid

VID = 0xCAFE
PID = 0x4000
REPORT_ID = 3
TOTAL_PACKET_LEN = 64
DATA_LEN = TOTAL_PACKET_LEN - 1  # 63 payload bytes
CMD_BOOTLOADER = 0xBB


def reboot_to_bootloader():
    device = hid.device()
    try:
        print(f"Connecting to device VID=0x{VID:04X}, PID=0x{PID:04X}...")
        device.open(VID, PID)
    except OSError as err:
        print(f"Error: Unable to open device: {err}")
        print("Make sure the keypad is plugged in and you have appropriate USB permissions (e.g., udev rules).")
        sys.exit(1)

    product = device.get_product_string() or "ZDC MacroPad"
    mfg = device.get_manufacturer_string() or "ZDC Engineering"
    print(f"Connected: {product} ({mfg})")

    # Construct the 64-byte feature report packet:
    # [Report ID (3)] + [0xBB (Bootloader Cmd)] + [0x00 * 62 padding]
    payload = [CMD_BOOTLOADER] + [0x00] * (DATA_LEN - 1)
    packet = [REPORT_ID] + payload

    print("Sending bootloader command (0xBB)...")

    try:
        res = device.send_feature_report(packet)
        # Note: The microcontroller reboots immediately via reset_usb_boot(0, 0),
        # so send_feature_report may return bytes transferred or raise an I/O error
        # due to immediate USB bus detachment. Both indicate success.
        if res < 0:
            print("Note: Transfer reported < 0 (expected during instantaneous reboot).")
    except OSError as err:
        print(f"Note: Device detached immediately ({err}).")
    finally:
        with suppress(OSError):
            device.close()

        print("Keypad rebooted into BOOTSEL mode (RPI-RP2 drive should appear shortly)!")


if __name__ == "__main__":
    reboot_to_bootloader()

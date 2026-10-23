#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="${SCRIPT_DIR}/hid_bootloader"
SRC="${SCRIPT_DIR}/hid_bootloader.c"

echo "=== Flashing Keypad Firmware ==="

if [ ! -f "$BIN" ]; then
    echo "Compiling ${BIN}..."
    gcc -O2 "$SRC" -o "$BIN"
fi

"$BIN" || echo "Notice: HID reset skipped (device may already be in BOOTSEL mode)."

echo "Proceeding to flash..."
sleep 0.5

"${SCRIPT_DIR}/flash.sh" Keypad
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="${SCRIPT_DIR}/hid_bootloader"
SRC="${SCRIPT_DIR}/hid_bootloader.c"

if [ ! -f "$BIN" ]; then
    echo "Compiling ${BIN}..."
    gcc -O2 "$SRC" -o "$BIN"
fi

echo "Triggering HID bootloader reset..."

if "$BIN"; then
    echo "Reset trigger sent via HID."
else
    echo "Notice: HID reset skipped (device not responding or already in BOOTSEL mode)."
fi
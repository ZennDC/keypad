#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-}"

if [ -z "$PORT" ]; then
    PORT=$(ls /dev/ttyACM* 2>/dev/null | head -n 1 || true)
fi

if [ -z "$PORT" ] || [ ! -e "$PORT" ]; then
    echo "Notice: No /dev/ttyACM port found (device may already be in BOOTSEL mode)."
    exit 0
fi

echo "Triggering 1200 bps touch on $PORT..."

stty -F "$PORT" 1200 hupcl 2>/dev/null || true

exec 3<>"$PORT" 2>/dev/null || true
exec 3>&- 2>/dev/null || true

echo "Reset trigger sent to $PORT."
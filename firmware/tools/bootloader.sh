#!/usr/bin/env bash
set -euo pipefail

# Find ttyACM port (checks for the active device or takes the first matching ttyACM*)
PORT="${1:-}"

if [ -z "$PORT" ]; then
    # Auto-detect first available ttyACM port
    PORT=$(ls /dev/ttyACM* 2>/dev/null | head -n 1 || true)
fi

if [ -z "$PORT" ] || [ ! -e "$PORT" ]; then
    echo "Error: No /dev/ttyACM port found." >&2
    exit 1
fi

echo "Triggering 1200 bps touch on $PORT..."

# 1200-baud touch: set baud rate, toggle DTR, and close
stty -F "$PORT" 1200 hupcl 2>/dev/null || true

# Briefly open and close the file descriptor to ensure the baud change is committed
exec 3<>"$PORT" 2>/dev/null || true
exec 3>&- 2>/dev/null || true

echo "Reset trigger sent to $PORT."
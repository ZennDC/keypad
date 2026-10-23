#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== Flashing BSP Test Firmware ==="

"${SCRIPT_DIR}/reset-cdc.sh" || true

echo "Proceeding to flash..."
sleep 0.5

"${SCRIPT_DIR}/flash.sh" test_bsp
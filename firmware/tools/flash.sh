#!/usr/bin/env bash
set -euo pipefail

# Determine script directory (tools/) and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# --- Step 0: Execute bootloader.sh in the same directory (tools/) ---
BOOTLOADER_SCRIPT="${SCRIPT_DIR}/bootloader.sh"

if [ -f "$BOOTLOADER_SCRIPT" ]; then
    echo "Running bootloader script: ${BOOTLOADER_SCRIPT}..."
    bash "$BOOTLOADER_SCRIPT"
    sleep 1 # Allow the USB bus a moment to detach and re-enumerate as RPI-RP2
else
    echo "Warning: '${BOOTLOADER_SCRIPT}' not found. Skipping bootloader trigger..." >&2
fi

# Path to your compiled UF2 binary anchored to project root
UF2_FILE="${PROJECT_ROOT}/build/bsp.uf2"

# Ensure the binary exists before doing anything
if [ ! -f "$UF2_FILE" ]; then
    echo "Error: Binary not found at '$UF2_FILE'. Run 'cmake --build build' from project root first." >&2
    exit 1
fi

echo "Searching for RPI-RP2 bootloader drive..."

# Wait up to 10 seconds for the RPI-RP2 block device to appear
DEV_PATH=""
for i in {1..20}; do
    DEV_PATH=$(lsblk -no PATH,LABEL 2>/dev/null | awk '$2=="RPI-RP2" {print $1; exit}' || true)
    if [ -n "$DEV_PATH" ]; then
        break
    fi
    sleep 0.5
done

if [ -z "$DEV_PATH" ]; then
    echo "Error: RPI-RP2 drive not found." >&2
    echo "Make sure the board is in BOOTSEL mode." >&2
    exit 1
fi

echo "Found block device: $DEV_PATH"

# Check if the device is already mounted by the desktop environment
MOUNT_POINT=$(lsblk -no MOUNTPOINT "$DEV_PATH" | tr -d '[:space:]')
CREATED_TEMP_DIR=false

if [ -z "$MOUNT_POINT" ]; then
    # Not mounted: create a temporary mount directory and mount it
    MOUNT_POINT=$(mktemp -d -t rpi_mount_XXXXXX)
    CREATED_TEMP_DIR=true
    echo "Mounting $DEV_PATH to $MOUNT_POINT..."
    sudo mount "$DEV_PATH" "$MOUNT_POINT"
else
    echo "Device already mounted at: $MOUNT_POINT"
fi

# Clean up temporary mount on script exit/abort
cleanup() {
    if [ "$CREATED_TEMP_DIR" = true ]; then
        echo "Cleaning up mount point..."
        sudo umount "$MOUNT_POINT" 2>/dev/null || true
        rmdir "$MOUNT_POINT" 2>/dev/null || true
    fi
}
trap cleanup EXIT

echo "Flashing '$UF2_FILE' to $MOUNT_POINT..."
if [ -w "$MOUNT_POINT" ]; then
    cp "$UF2_FILE" "$MOUNT_POINT/"
else
    sudo cp "$UF2_FILE" "$MOUNT_POINT/"
fi

# Sync to ensure the write finishes completely
sync

echo "Flash complete! The RP2040 is rebooting into your firmware."
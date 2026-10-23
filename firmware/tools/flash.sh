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

# Check if the device is already mounted
MOUNT_POINT=$(lsblk -no MOUNTPOINT "$DEV_PATH" | tr -d '[:space:]')
MOUNTED_BY_SCRIPT=false

if [ -z "$MOUNT_POINT" ]; then
    echo "Mounting $DEV_PATH via udisksctl..."
    # udisksctl mounts user-space into /media/$USER/... or /run/media/$USER/...
    MOUNT_OUTPUT=$(udisksctl mount -b "$DEV_PATH")
    MOUNT_POINT=$(echo "$MOUNT_OUTPUT" | awk '{print $NF}' | tr -d '.')
    MOUNTED_BY_SCRIPT=true
    echo "Mounted to: $MOUNT_POINT"
else
    echo "Device already mounted at: $MOUNT_POINT"
fi

cleanup() {
    if [ "$MOUNTED_BY_SCRIPT" = true ]; then
        echo "Cleaning up mount point..."
        # Unmount if still present (device often auto-detaches once flashed)
        udisksctl unmount -b "$DEV_PATH" 2>/dev/null || true
    fi
}
trap cleanup EXIT

echo "Flashing '$UF2_FILE' to $MOUNT_POINT..."
cp "$UF2_FILE" "$MOUNT_POINT/"

# Sync to ensure write is flushed
sync

echo "Flash complete! The RP2040 is rebooting into your firmware."
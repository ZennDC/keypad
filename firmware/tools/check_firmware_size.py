#!/usr/bin/env python3
import os
import subprocess
import sys

# RP2040 Hardware Limits
MAX_FLASH_BYTES = 2 * 1024 * 1024  # 2MB
MAX_SRAM_BYTES = 264 * 1024        # 264KB

def get_elf_sizes(elf_path: str, size_tool: str = "arm-none-eabi-size"):
    cmd = [size_tool, "-B", elf_path]
    try:
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True)
    except FileNotFoundError:
        # Fallback to system 'size' if arm-none-eabi-size is not in PATH
        cmd[0] = "size"
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True)

    lines = output.strip().splitlines()
    if len(lines) < 2:
        raise RuntimeError(f"Unexpected size output: {output}")

    # Format: text data bss dec hex filename
    parts = lines[1].split()
    text = int(parts[0])
    data = int(parts[1])
    bss = int(parts[2])
    return text, data, bss

def main():
    if len(sys.argv) < 2:
        print("Usage: check_firmware_size.py <path_to_elf> [size_binary]")
        sys.exit(1)

    elf_path = sys.argv[1]
    size_tool = sys.argv[2] if len(sys.argv) > 2 else "arm-none-eabi-size"

    if not os.path.isfile(elf_path):
        print(f"Error: ELF file not found: {elf_path}", file=sys.stderr)
        sys.exit(1)

    text, data, bss = get_elf_sizes(elf_path, size_tool)

    flash_used = text + data
    sram_used = data + bss

    flash_pct = (flash_used / MAX_FLASH_BYTES) * 100
    sram_pct = (sram_used / MAX_SRAM_BYTES) * 100

    flash_ok = flash_used <= MAX_FLASH_BYTES
    sram_ok = sram_used <= MAX_SRAM_BYTES

    print(f"Firmware Binary: {elf_path}")
    print(f"  Flash (ROM): {flash_used:,} / {MAX_FLASH_BYTES:,} bytes ({flash_pct:.2f}%) -> {'OK' if flash_ok else 'EXCEEDED'}")
    print(f"  SRAM  (RAM): {sram_used:,} / {MAX_SRAM_BYTES:,} bytes ({sram_pct:.2f}%) -> {'OK' if sram_ok else 'EXCEEDED'}")

    # Emit to GitHub Step Summary if running in GitHub Actions
    summary_file = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_file:
        flash_status = "✅ OK" if flash_ok else "❌ EXCEEDED"
        sram_status = "✅ OK" if sram_ok else "❌ EXCEEDED"
        summary_md = f"""### 🛡️ Firmware Memory Footprint Guard

| Memory Region | Used (Bytes) | Total (Bytes) | Usage | Status |
|:---|:---:|:---:|:---:|:---:|
| **Flash (ROM)** | {flash_used:,} B | {MAX_FLASH_BYTES:,} B (2 MB) | {flash_pct:.2f}% | {flash_status} |
| **SRAM (RAM)** | {sram_used:,} B | {MAX_SRAM_BYTES:,} B (264 KB) | {sram_pct:.2f}% | {sram_status} |

<details>
<summary>Detailed Section Sizes</summary>

- `.text` (code + const data): {text:,} bytes
- `.data` (initialized data in RAM): {data:,} bytes
- `.bss` (zero-initialized static data): {bss:,} bytes

</details>
"""
        with open(summary_file, "a", encoding="utf-8") as f:
            f.write(summary_md + "\n")

    if not flash_ok:
        print(f"ERROR: Firmware Flash usage ({flash_used:,} B) exceeds maximum limit ({MAX_FLASH_BYTES:,} B)!", file=sys.stderr)
        sys.exit(1)

    if not sram_ok:
        print(f"ERROR: Firmware SRAM usage ({sram_used:,} B) exceeds maximum limit ({MAX_SRAM_BYTES:,} B)!", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()

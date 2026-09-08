import hid

VID = 0xCafe
PID = 0x4000
REPORT_ID = 3
CONFIG_PACKET_SIZE = 64  # 1 byte ID + 63 bytes payload

device = hid.device()
device.open(VID, PID)

raw_data = device.get_feature_report(REPORT_ID, CONFIG_PACKET_SIZE)
if not raw_data:
    raise RuntimeError("Failed to read report from device.")

# Strip report_id if hidapi includes it at index 0
data = raw_data[1:] if raw_data[0] == REPORT_ID else raw_data

type_labels = {0: "Disabled", 1: "Keyboard", 2: "Consumer"}

# 1. Parse Keys First (Bytes 0 - 47)
print("=== Key Mappings (0 - 15) ===")
offset = 0
for i in range(16):
    rpt_type = data[offset]
    usage = data[offset + 1] | (data[offset + 2] << 8)
    offset += 3
    
    label = type_labels.get(rpt_type, f"Unknown ({rpt_type})")
    print(f"Key {i:2d}: [{label:<8}] Usage Code: 0x{usage:04X}")

# 2. Parse Screen Second (Bytes 48 - 50)
display_mode = data[offset]
display_orientation = data[offset + 1]
display_brightness = data[offset + 2]
offset += 3

print("\n=== Display Settings ===")
print(f"Mode:        {display_mode}")
print(f"Orientation: {display_orientation}")
print(f"Brightness:  {display_brightness}%")

# 3. Reserved / Extra Bytes (Bytes 51 - 62)
reserved_bytes = data[offset:63]
print(f"\nReserved Bytes Available: {len(reserved_bytes)} bytes")

device.close()
import time

import hid

VID = 0xCafe
PID = 0x4000
REPORT_ID = 3
TOTAL_PACKET_LEN = 64
DATA_LEN = TOTAL_PACKET_LEN - 1  # 63 payload bytes

dev = hid.device()
dev.open(VID, PID)

product = dev.get_product_string() or "ZDC MacroPad"
mfg = dev.get_manufacturer_string() or "ZDC Engineering"
print(f"Connected: {product} ({mfg})\n")

def send_feature(payload: list[int]):
    padded = payload + [0x00] * (DATA_LEN - len(payload))
    packet = [REPORT_ID] + padded
    dev.send_feature_report(packet)
    time.sleep(0.01)  # Allow USB control transfer to complete

# 1. Define Test Data
expected = {
    "display": {"mode": 1, "orientation": 2, "brightness": 85},
    "keys": {
        0:  {"type": 1, "usage": 0x0004, "desc": "Keyboard 'A' (0x0004)"},
        5:  {"type": 2, "usage": 0x00E2, "desc": "Consumer 'Mute' (0x00E2)"},
        15: {"type": 2, "usage": 0x00E9, "desc": "Consumer 'Volume Up' (0x00E9)"},
    }
}

# 2. Send New Configuration
print("--- Sending New Configuration ---")
d = expected["display"]
print(f"• Set Display: Mode={d['mode']}, Orientation={d['orientation']}, Brightness={d['brightness']}%")
send_feature([0xFF, d["mode"], d["orientation"], d["brightness"]])

for idx, k in expected["keys"].items():
    print(f"• Set Key {idx}: {k['desc']}")
    usage_lsb = k["usage"] & 0xFF
    usage_msb = (k["usage"] >> 8) & 0xFF
    send_feature([idx, k["type"], usage_lsb, usage_msb])

# 3. Read Back Device State
print("\n--- Reading Back Current Device State ---")
raw = dev.get_feature_report(REPORT_ID, TOTAL_PACKET_LEN)
data = raw[1:] if raw[0] == REPORT_ID else raw

# Unpack keys (Bytes 0 - 47)
read_keys = {}
for i in range(16):
    rpt_type = data[i * 3]
    usage = data[i * 3 + 1] | (data[i * 3 + 2] << 8)
    read_keys[i] = {"type": rpt_type, "usage": usage}

# Unpack display (Bytes 48 - 50)
read_display = {
    "mode": data[48],
    "orientation": data[49],
    "brightness": data[50]
}

for idx in expected["keys"]:
    print(f"Key {idx:2d}: Type={read_keys[idx]['type']}, Usage=0x{read_keys[idx]['usage']:04X}")

print(f"Display: Mode={read_display['mode']}, Orientation={read_display['orientation']}, Brightness={read_display['brightness']}%")

# 4. Verify Results
print("\n--- Verification Result ---")
all_passed = True

def check(field_name: str, expected_val, actual_val):
    global all_passed
    if expected_val == actual_val:
        print(f"PASS: {field_name} matches ({actual_val})")
    else:
        all_passed = False
        print(f"FAIL: {field_name} expected {expected_val}, got {actual_val}")

for idx, exp in expected["keys"].items():
    act = read_keys[idx]
    check(f"Key {idx} Report Type", exp["type"], act["type"])
    check(f"Key {idx} Usage Code", exp["usage"], act["usage"])

check("Display Mode", expected["display"]["mode"], read_display["mode"])
check("Display Orientation", expected["display"]["orientation"], read_display["orientation"])
check("Display Brightness", expected["display"]["brightness"], read_display["brightness"])

dev.close()
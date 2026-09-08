import hid

VID = 0xCafe
PID = 0x4000
REPORT_ID = 3
TOTAL_PACKET_SIZE = 64
DATA_PAYLOAD_SIZE = TOTAL_PACKET_SIZE - 1  # 63 bytes

def send_config(device, cmd_bytes: list[int]):
    if len(cmd_bytes) > DATA_PAYLOAD_SIZE:
        raise ValueError(f"Payload too large! Max {DATA_PAYLOAD_SIZE} bytes.")

    # Pad data to exactly 63 bytes
    padded_data = cmd_bytes + [0x00] * (DATA_PAYLOAD_SIZE - len(cmd_bytes))
    
    # 1 byte ID + 63 bytes data = 64 bytes total
    packet = [REPORT_ID] + padded_data
    
    bytes_sent = device.send_feature_report(packet)
    if bytes_sent < 0:
        raise RuntimeError("Failed to send feature report.")
    return bytes_sent

# Usage
device = hid.device()
device.open(VID, PID)
print(f"Connected to: {device.get_product_string()}")

# Example: Command 0x01 (Brightness), Value 80%
bytes_written = send_config(device, [0x01, 80])
print(f"Config successfully sent ({bytes_written} bytes)!")

device.close()
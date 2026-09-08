import time

import hid

VID = 0xCafe
PID = 0x4000
REPORT_ID = 3
TOTAL_PACKET_LEN = 64
DATA_LEN = TOTAL_PACKET_LEN - 1

def send_feature(dev, payload):
    padded = payload + [0x00] * (DATA_LEN - len(payload))
    dev.send_feature_report([REPORT_ID] + padded)
    time.sleep(0.01)

dev = hid.device()
dev.open(VID, PID)

# 1. Send settings
send_feature(dev, [0xFF, 1, 2, 80])                 # Display
send_feature(dev, [0, 1, 0x04, 0x00])               # Key 0 -> 'A'
send_feature(dev, [5, 2, 0xE2, 0x00])               # Key 5 -> Mute
send_feature(dev, [15, 2, 0xE9, 0x00])              # Key 15 -> Vol Up

# 2. Commit changes to Pico onboard flash
print("Committing to onboard flash...")
send_feature(dev, [0xAA])                           # CMD_SAVE_TO_FLASH
time.sleep(0.05)                                    # Give flash erase/write 50ms to finish

print("Settings committed. Unplug the Pico, plug it back in, and run your read script!")
dev.close()
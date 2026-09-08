import ctypes
import os

import hid

# Grab libc's errno location
libc = ctypes.CDLL(None)
get_errno_loc = libc.__errno_location
get_errno_loc.restype = ctypes.POINTER(ctypes.c_int)


def get_last_error():
    err_num = get_errno_loc().contents.value
    return f"Errno {err_num}: {os.strerror(err_num)}"


VID = 0xCAFE
PID = 0x4000
REPORT_ID = 3

device = hid.device()
device.open(VID, PID)
print(f"Connected to: {device.get_product_string()} ({device.get_manufacturer_string()})")

# Test sending 63 data bytes (total packet = 64 bytes with Report ID)
payload_63 = [REPORT_ID] + [0x42] * 63
res = device.send_feature_report(payload_63)

if res < 0:
    print("Sending 64 total bytes (1 ID + 63 data) FAILED ->", get_last_error())
else:
    print(f"Sending 64 total bytes SUCCEEDED! ({res} bytes transferred)")

# Test sending 64 data bytes (total packet = 65 bytes with Report ID)
payload_64 = [REPORT_ID] + [0x42] * 64
res = device.send_feature_report(payload_64)

if res < 0:
    print("Sending 65 total bytes (1 ID + 64 data) FAILED ->", get_last_error())
else:
    print(f"Sending 65 total bytes SUCCEEDED! ({res} bytes transferred)")

device.close()

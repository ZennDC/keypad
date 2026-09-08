#include <iostream>
#include <array>
#include <cstdint>
#include <hidapi/hidapi.h>
#include "protocol.h"

constexpr uint16_t VID = 0xCafe;
constexpr uint16_t PID = 0x4000;

int main() {
    std::cout << "1. Initializing HIDAPI..." << std::endl;
    if (hid_init() != 0) {
        std::cerr << "Failed to init hidapi" << std::endl;
        return 1;
    }

    std::cout << "2. Opening device..." << std::endl;
    hid_device *device = hid_open(VID, PID, nullptr);
    if (!device) {
        std::cerr << "Could not open device" << std::endl;
        hid_exit();
        return 1;
    }

    wchar_t prod[256];
    if (hid_get_product_string(device, prod, 256) == 0) {
        std::wcout << L"Connected to: " << prod << std::endl;
    }

    std::cout << "3. Preparing packet..." << std::endl;
    std::array<uint8_t, CONFIG_REPORT_SIZE> packet{};
    packet[0] = REPORT_ID_CONFIG_FEATURE; // 3
    packet[1] = CMD_CONFIG_TARGET_DISPLAY; // 0xFF
    packet[2] = 0x00;
    packet[3] = 0x00;
    packet[4] = 20; // 20% brightness

    std::cout << "4. Sending feature report (" << packet.size() << " bytes)..." << std::endl;

    int res = hid_send_feature_report(device, packet.data(), packet.size());

    std::cout << "5. Call returned with code: " << res << std::endl;

    if (res < 0) {
        const wchar_t *err = hid_error(device);
        std::wcout << L"HID Send Failed: " << (err ? err : L"unknown") << std::endl;
    } else {
        std::cout << "Success! Wrote " << res << " bytes." << std::endl;
    }

    hid_close(device);
    hid_exit();
    return 0;
}
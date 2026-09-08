#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <hidapi/hidapi.h>

#define VID 0xCafe
#define PID 0x4000
#define REPORT_ID 3

// Print CLI usage guide
static void print_usage(const char *prog) {
    printf("Usage:\n");
    printf("  %s key <switch_0_15> <type: 1=keyboard|2=consumer> <hex_usage_code>\n", prog);
    printf("  %s display <mode_0_4> <orientation_0_3> <brightness_0_100>\n", prog);
    printf("\nExamples:\n");
    printf("  %s key 0 2 E2           # Set SW1 to Mute (Consumer 0xE0E2)\n", prog);
    printf("  %s key 5 1 28           # Set SW6 to Enter (Keyboard 0x28)\n", prog);
    printf("  %s display 0 1 80       # Hardware Monitor, Landscape, 80%% brightness\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (hid_init() != 0) {
        fprintf(stderr, "Error: Unable to initialize HIDAPI.\n");
        return 1;
    }

    // Open connection to the Pico
    hid_device *dev = hid_open(VID, PID, NULL);
    if (!dev) {
        fprintf(stderr, "Error: Pico MacroDeck not found (0x%04X:0x%04X). Check permissions/connection.\n", VID, PID);
        hid_exit();
        return 1;
    }

    // Buffer layout: [0] = Report ID (0x03), [1..64] = Payload
    uint8_t report_buf[65] = {0};
    report_buf[0] = REPORT_ID;

    if (strcmp(argv[1], "key") == 0) {
        if (argc != 5) {
            print_usage(argv[0]);
            hid_close(dev);
            hid_exit();
            return 1;
        }

        int key_index = atoi(argv[2]);
        int key_type  = atoi(argv[3]);
        int key_code  = (int)strtol(argv[4], NULL, 16);

        if (key_index < 0 || key_index > 15) {
            fprintf(stderr, "Error: Switch index must be between 0 and 15.\n");
            hid_close(dev);
            hid_exit();
            return 1;
        }

        report_buf[1] = (uint8_t)key_index;
        report_buf[2] = (uint8_t)key_type;
        report_buf[3] = (uint8_t)(key_code & 0xFF);

        int res = hid_send_feature_report(dev, report_buf, sizeof(report_buf));
        if (res < 0) {
            fprintf(stderr, "Failed to send key config. Error: %ls\n", hid_error(dev));
        } else {
            printf("Successfully mapped SW %d -> Type %d, Code 0x%02X\n", key_index + 1, key_type, key_code & 0xFF);
        }

    } else if (strcmp(argv[1], "display") == 0) {
        if (argc != 5) {
            print_usage(argv[0]);
            hid_close(dev);
            hid_exit();
            return 1;
        }

        int mode        = atoi(argv[2]);
        int orientation = atoi(argv[3]);
        int brightness  = atoi(argv[4]);

        if (brightness < 0 || brightness > 100) {
            fprintf(stderr, "Error: Brightness must be between 0 and 100.\n");
            hid_close(dev);
            hid_exit();
            return 1;
        }

        report_buf[1] = 0xFF; // Target 0xFF indicates display packet
        report_buf[2] = (uint8_t)mode;
        report_buf[3] = (uint8_t)orientation;
        report_buf[4] = (uint8_t)brightness;

        int res = hid_send_feature_report(dev, report_buf, sizeof(report_buf));
        if (res < 0) {
            fprintf(stderr, "Failed to send display config. Error: %ls\n", hid_error(dev));
        } else {
            printf("Successfully updated ST7789: Mode=%d, Orientation=%d, Brightness=%d%%\n", mode, orientation, brightness);
        }

    } else {
        print_usage(argv[0]);
    }

    hid_close(dev);
    hid_exit();
    return 0;
}
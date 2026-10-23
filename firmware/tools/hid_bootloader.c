#include <dirent.h>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TARGET_VID      0xCAFE
#define TARGET_PID      0x4000
#define REPORT_ID       3
#define CMD_BOOTLOADER  0xBB
#define REPORT_LEN      64

static bool find_hidraw_device(char *out_path, size_t max_len) {
    const char *base_path = "/sys/class/hidraw";
    DIR *dir = opendir(base_path);
    if (!dir) return false;

    struct dirent *entry;
    bool found = false;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "hidraw", 6) != 0) continue;

        char uevent_path[PATH_MAX];
        snprintf(uevent_path, sizeof(uevent_path), "%s/%s/device/uevent", base_path, entry->d_name);

        FILE *f = fopen(uevent_path, "r");
        if (!f) continue;

        char line[256];
        while (fgets(line, sizeof(line), f)) {
            unsigned int bus = 0, vid = 0, pid = 0;
            if (sscanf(line, "HID_ID=%x:%x:%x", &bus, &vid, &pid) == 3) {
                if (vid == TARGET_VID && pid == TARGET_PID) {
                    snprintf(out_path, max_len, "/dev/%s", entry->d_name);
                    found = true;
                    break;
                }
            }
        }
        fclose(f);
        if (found) break;
    }

    closedir(dir);
    return found;
}

int main(void) {
    char dev_path[PATH_MAX];

    printf("Searching for ZDC MacroPad (VID: 0x%04X, PID: 0x%04X)...\n", TARGET_VID, TARGET_PID);

    if (!find_hidraw_device(dev_path, sizeof(dev_path))) {
        fprintf(stderr, "Error: ZDC MacroPad not found. Check USB connection and permissions.\n");
        return 1;
    }

    printf("Found device node at %s\n", dev_path);

    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        perror("Error opening hidraw device");
        return 1;
    }

    uint8_t buffer[REPORT_LEN] = {0};
    buffer[0] = REPORT_ID;
    buffer[1] = CMD_BOOTLOADER;

    printf("Sending bootloader command (0xBB)...\n");

    // RP2040 reboots instantly, so ioctl may return < 0 or fail with ENODEV as USB detaches
    ioctl(fd, HIDIOCSFEATURE(REPORT_LEN), buffer);

    close(fd);
    printf("Keypad rebooted into BOOTSEL mode!\n");
    return 0;
}
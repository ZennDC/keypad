#include "usb_descriptors.h"
#include <string.h>

// -----------------------------------------------------------------------------
// 1. Device Descriptor
// -----------------------------------------------------------------------------
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0xCafe,
    .idProduct = 0x4000,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

// -----------------------------------------------------------------------------
// 2. HID Report Descriptor
// -----------------------------------------------------------------------------
#define TUD_HID_REPORT_DESC_VENDOR_FEATURE(report_id, size)  \
    HID_USAGE_PAGE_N(0xFF00, 2),                             \
        HID_USAGE(0x01),                                     \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),          \
        HID_REPORT_ID(report_id)                             \
            HID_USAGE(0x02),                                 \
        HID_LOGICAL_MIN(0x00),                               \
        HID_LOGICAL_MAX(0xFF),                               \
        HID_REPORT_SIZE(8),                                  \
        HID_REPORT_COUNT(size),                              \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_COLLECTION_END

uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL)),
    TUD_HID_REPORT_DESC_VENDOR_FEATURE(REPORT_ID_CONFIG_FEATURE, CONFIG_REPORT_SIZE)};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return desc_hid_report;
}

// -----------------------------------------------------------------------------
// 3. Configuration Descriptor
// -----------------------------------------------------------------------------
#define EPNUM_HID 0x81
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 10)};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

// -----------------------------------------------------------------------------
// 4. String Descriptors
// -----------------------------------------------------------------------------
static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: English (0x0409)
    "ZDC Engineering",          // 1: Manufacturer
    "ZDC MacroPad",             // 2: Product
    "ZDC-MP16-0001"             // 3: Serial
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    uint8_t chr_count;

    if (index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
            return NULL;
        const char *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31)
            chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++)
        {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}

// -----------------------------------------------------------------------------
// 5. HID Callbacks
// -----------------------------------------------------------------------------
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, 
                               hid_report_type_t report_type, 
                               uint8_t *buffer, uint16_t reqlen) {
    (void) instance;

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == REPORT_ID_CONFIG_FEATURE) {
        memset(buffer, 0, reqlen);
        uint16_t offset = 0;

        // 1. Keys first: 16 keys * 3 bytes = 48 bytes (Bytes 0 - 47)
        for (int i = 0; i < 16; i++) {
            buffer[offset++] = keys[i].report_type;
            buffer[offset++] = (uint8_t)(keys[i].usage_code & 0xFF);
            buffer[offset++] = (uint8_t)((keys[i].usage_code >> 8) & 0xFF);
        }

        // 2. Screen second: 3 bytes (Bytes 48 - 50)
        buffer[offset++] = display_cfg.mode;
        buffer[offset++] = display_cfg.orientation;
        buffer[offset++] = display_cfg.brightness;

        // Bytes 51 - 62 (12 bytes) remain zeroed for future features
        return reqlen;
    }

    memset(buffer, 0, reqlen);
    return reqlen;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == REPORT_ID_CONFIG_FEATURE)
    {
        handle_keypad_config_packet(buffer, bufsize);
    }
}
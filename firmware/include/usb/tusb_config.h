#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------+
// Port Configuration
// --------------------------------------------------------------------+
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// --------------------------------------------------------------------+
// Device Core Configuration
// --------------------------------------------------------------------+
#define CFG_TUD_ENABLED 1
#define CFG_TUD_ENDPOINT0_SIZE 64

// --------------------------------------------------------------------+
// HID Class Driver Configuration
// --------------------------------------------------------------------+
#define CFG_TUD_HID 1
#define CFG_TUD_HID_EP_BUFSIZE 64

#ifdef __cplusplus
}
#endif

#endif // _TUSB_CONFIG_H_
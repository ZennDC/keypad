#include "usb_descriptors.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include <stddef.h>
#include <string.h>

// Target the last 4KB sector of 2MB flash
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define FLASH_TARGET_ADDR   ((const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET))

static uint8_t calculate_checksum(const persistent_config_t *cfg) {
    const uint8_t *p = (const uint8_t *)cfg;
    uint8_t sum = 0;
    // checksum is not necessarily the last byte: the compiler may append
    // alignment padding to persistent_config_t. Only cover bytes before the
    // checksum field, otherwise a saved record can never validate on reboot.
    for (size_t i = 0; i < offsetof(persistent_config_t, checksum); i++) {
        sum += p[i];
    }
    return sum;
}

void flash_storage_init(void) {
    const persistent_config_t *stored = (const persistent_config_t *)FLASH_TARGET_ADDR;

    // Check if flash contains valid saved data
    if (stored->magic == FLASH_CONFIG_MAGIC && 
        stored->version == FLASH_CONFIG_VERSION &&
        stored->checksum == calculate_checksum(stored)) 
    {
        // Valid save found: Restore settings into active RAM
        display_cfg = stored->display_cfg;
        for (int i = 0; i < 16; i++) {
            keys[i].report_type = stored->keys[i].report_type;
            keys[i].usage_code  = stored->keys[i].usage_code;
            keys[i].was_pressed = false; // Never restore volatile key press state
        }
        set_display_brightness(display_cfg.brightness);
    } 
    else 
    {
        // First boot / blank flash: Save default settings immediately
        flash_storage_save();
    }
}

void flash_storage_save(void) {
    // 1. Prepare flash buffer aligned to 4KB sector size
    uint8_t sector_buf[FLASH_SECTOR_SIZE];
    memset(sector_buf, 0xFF, sizeof(sector_buf));

    persistent_config_t *cfg = (persistent_config_t *)sector_buf;
    cfg->magic = FLASH_CONFIG_MAGIC;
    cfg->version = FLASH_CONFIG_VERSION;
    cfg->display_cfg = display_cfg;

    for (int i = 0; i < 16; i++) {
        cfg->keys[i].report_type = keys[i].report_type;
        cfg->keys[i].usage_code  = keys[i].usage_code;
        cfg->keys[i].was_pressed = false;
    }
    cfg->checksum = calculate_checksum(cfg);

    // 2. CRITICAL SECTION: Disable interrupts to prevent executing code from flash while erasing/programming
    uint32_t ints = save_and_disable_interrupts();
    
    // Erase 1 sector (4096 bytes)
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    
    // Program sector in multiples of FLASH_PAGE_SIZE (256 bytes)
    flash_range_program(FLASH_TARGET_OFFSET, sector_buf, FLASH_SECTOR_SIZE);
    
    restore_interrupts(ints);
}

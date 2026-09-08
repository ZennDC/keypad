#include "flash_storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
static const persistent_config_t *flash_target = (const persistent_config_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

static uint8_t calculate_checksum(const persistent_config_t *cfg) {
    uint8_t sum = 0;
    const uint8_t *bytes = (const uint8_t *)cfg;
    // Sum all bytes up to (but not including) the checksum field
    size_t len = offsetof(persistent_config_t, checksum);
    for (size_t i = 0; i < len; i++) {
        sum += bytes[i];
    }
    return sum;
}

void flash_storage_init(void) {
    persistent_config_t cfg;
    if (!flash_storage_read(&cfg)) {
        flash_storage_reset_defaults();
    }
}

bool flash_storage_read(persistent_config_t *out_config) {
    if (!out_config) return false;

    if (flash_target->magic != FLASH_CONFIG_MAGIC || flash_target->version != FLASH_CONFIG_VERSION) {
        return false;
    }

    if (flash_target->checksum != calculate_checksum(flash_target)) {
        return false;
    }

    memcpy(out_config, flash_target, sizeof(persistent_config_t));
    return true;
}

bool flash_storage_write(const persistent_config_t *in_config) {
    if (!in_config) return false;

    // Buffer sized to a flash page (256 bytes) to satisfy RP2040 programming requirements
    uint8_t buffer[FLASH_PAGE_SIZE];
    memset(buffer, 0xFF, sizeof(buffer));

    persistent_config_t *cfg = (persistent_config_t *)buffer;
    memcpy(cfg, in_config, sizeof(persistent_config_t));

    cfg->magic = FLASH_CONFIG_MAGIC;
    cfg->version = FLASH_CONFIG_VERSION;
    cfg->checksum = calculate_checksum(cfg);

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_PAGE_SIZE);
    restore_interrupts(ints);

    return true;
}

void flash_storage_reset_defaults(void) {
    persistent_config_t defaults = {
        .magic = FLASH_CONFIG_MAGIC,
        .version = FLASH_CONFIG_VERSION,
        .display_cfg = {
            .mode = 0,
            .orientation = 0,
            .brightness = 80
        },
        .keys = {0}
    };
    flash_storage_write(&defaults);
}
#ifndef FLASH_STORAGE_H_
#define FLASH_STORAGE_H_

#include "protocol.h"
#include <stdbool.h>

#define FLASH_CONFIG_MAGIC    0x4D414352  // "MACR"
#define FLASH_CONFIG_VERSION  1

void flash_storage_init(void);
bool flash_storage_read(persistent_config_t *out_config);
bool flash_storage_write(const persistent_config_t *in_config);
void flash_storage_reset_defaults(void);

#endif // FLASH_STORAGE_H_
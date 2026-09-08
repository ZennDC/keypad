#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_

#include "protocol.h"
#include <stdint.h>

void config_manager_init(void);
st7789_config_t *config_manager_get_display(void);
macro_key_t *config_manager_get_keys(void);
void config_manager_handle_packet(const uint8_t *buffer, uint16_t bufsize);

#endif // CONFIG_MANAGER_H_
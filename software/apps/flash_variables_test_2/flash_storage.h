#include <stddef.h>
#include <stdint.h>
#include "nrf.h"

void flash_storage_init();

uint32_t define_flash_variable(uint32_t initial_value, uint16_t record_key, size_t size);

ret_code_t flash_update(uint16_t record_key, uint32_t value, size_t size);
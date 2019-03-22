#include <stddef.h>
#include "nrf.h"

void flash_storage_init();

ret_code_t flash_put(const char *name, void const *p_data, size_t data_size);

uint32_t flash_get(const char *name);

#include <stddef.h>
#include <stdint.h>
#include "nrf.h"

void flash_storage_init();

int define_flash_variable_int(int initial_value, uint16_t record_key);

float define_flash_variable_float(float initial_value, uint16_t record_key);

void define_flash_variable_string(char *initial_value, char *dest, uint16_t record_key);

ret_code_t flash_update_int(uint16_t record_key, int value);

ret_code_t flash_update_float(uint16_t record_key, int value);

ret_code_t flash_update_string(uint16_t record_key, char* value);
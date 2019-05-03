#include <stddef.h>
#include <stdint.h>
#include "nrf.h"

void flash_storage_init();

int define_flash_variable_int(const int initial_value, uint16_t record_key);

float define_flash_variable_float(const float initial_value, uint16_t record_key);

void define_flash_variable_array(const uint8_t* initial_value, uint8_t *dest, const size_t length, uint16_t record_key);

ret_code_t flash_update_int(const uint16_t record_key, const int value);

ret_code_t flash_update_float(const uint16_t record_key, const float value);

ret_code_t flash_update_array(const uint16_t record_key, const char* value, const size_t length);

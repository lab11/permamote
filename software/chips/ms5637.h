// Datasheet: http://www.te.com/commerce/DocumentDelivery/DDEController?Action=srchrtrv&DocNm=MS5637-02BA03&DocType=Data+Sheet&DocLang=English
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nrf_twi_mngr.h"

#define MS5637_ADDR 0x76

#define MS5637_RESET  0x1E
#define MS5637_PROM   0xA2

typedef enum {
  osr_8192,
  osr_4096,
  osr_2048,
  osr_1024,
  osr_512,
  osr_256
} MS5637_OSR;

void  ms5637_init(const nrf_twi_mngr_t* instance, MS5637_OSR osr);
void  ms5637_start(void);
void  ms5637_get_temperature_and_pressure(float* temp, float* pres);

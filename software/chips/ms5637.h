// Datasheet: http://www.te.com/commerce/DocumentDelivery/DDEController?Action=srchrtrv&DocNm=MS5637-02BA03&DocType=Data+Sheet&DocLang=English

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_drv_twi.h"

#define MS5637_ADDR 0x76

typedef enum {
  osr_8192,
  osr_4096,
  osr_2048,
  osr_1024,
  osr_512,
  osr_256
} OSR;

void ms5637_init(nrf_drv_twi_t* instance);
void ms5637_start(void);
float ms5637_get_temperature(OSR osr);
float ms5637_get_pressure(OSR osr);

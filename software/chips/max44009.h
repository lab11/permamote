// Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX44009.pdf

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_twi_mngr.h"

#define MAX44009_ADDR       0x4A
#define MAX44009_CONFIG     0x02
#define MAX44009_LUX_HIGH   0x03
#define MAX44009_LUX_LOW    0x04

typedef void max44009_read_lux_callback(float lux_result);

typedef struct {
  bool continuous;  // enable continuous sample mode
  bool manual;      // allow manual IC configuration
  bool cdr;         // perform current division for high brightness
  uint8_t int_time; // integration timing, (automatically set if manual = 0)
} max44009_config_t;

void max44009_init(const nrf_twi_mngr_t* instance, max44009_read_lux_callback* callback);
void max44009_config(max44009_config_t config);
void max44009_schedule_read_lux(void);

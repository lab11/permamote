// Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX44009.pdf

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_twi_mngr.h"

#define MAX44009_ADDR       0x4A
#define MAX44009_INT_STATUS 0x00
#define MAX44009_INT_EN     0x01
#define MAX44009_CONFIG     0x02
#define MAX44009_LUX_HI     0x03
#define MAX44009_LUX_LO     0x04
#define MAX44009_THRESH_HI  0x05
#define MAX44009_THRESH_LO  0x06
#define MAX44009_INT_TIME   0x07

typedef void max44009_read_lux_callback(float lux);
typedef void max44009_interrupt_callback(void);

typedef struct {
  bool continuous;  // enable continuous sample mode
  bool manual;      // allow manual IC configuration
  bool cdr;         // perform current division for high brightness
  uint8_t int_time; // integration timing, (automatically set if manual = 0)
} max44009_config_t;

void  max44009_init(const nrf_twi_mngr_t* instance);
void  max44009_set_interrupt_callback(max44009_interrupt_callback* callback);
void  max44009_enable_interrupt(void);
void  max44009_disable_interrupt(void);
void  max44009_config(max44009_config_t config);
void  max44009_set_read_lux_callback(max44009_read_lux_callback* callback);
void  max44009_set_upper_threshold(float thresh);
void  max44009_set_lower_threshold(float thresh);
void  max44009_schedule_read_lux(void);
float max44009_read_lux(void);

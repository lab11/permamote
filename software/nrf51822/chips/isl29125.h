// Datasheet: https://www.intersil.com/content/dam/Intersil/documents/isl2/isl29125.pdf

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_drv_twi.h"

#define ISL29125_ADDR   0x44
#define ISL29125_ID_RST 0x00
#define ISL29125_CFG1   0x01
#define ISL29125_CFG2   0x02
#define ISL29125_CFG3   0x03
#define ISL29125_STATUS 0x08
#define ISL29125_G_LOW  0x09
#define ISL29125_G_HIGH 0x0A
#define ISL29125_R_LOW  0x0B
#define ISL29125_R_HIGH 0x0C
#define ISL29125_B_LOW  0x0D
#define ISL29125_B_HIGH 0x0E

typedef enum {
  isl29125_power_down     = 0,
  isl29125_green          = 1,
  isl29125_red            = 2,
  isl29125_blue           = 3,
  isl29125_standby        = 4,
  isl29125_green_red_blue = 5,
  isl29125_green_red      = 6,
  isl29125_green_blue     = 7
} isl29125_mode_t;

typedef struct {
  isl29125_mode_t mode;
  bool range;           // 0 = range of 375 lux, 1 = range of 10000 lux
  bool resolution;      // 0 = 16 bit resolution, 1 = 12 bit resolution
  bool sync_int;        // 0 = adc start at i2c write 0x01, 1 = adc start at rising int
} isl29125_config_t;

void isl29125_init(nrf_drv_twi_t* instance);
void isl29125_config(isl29125_config_t config);
void isl29125_read_lux(float* red, float* green, float* blue);

#include <math.h>

#include "isl29125.h"

static nrf_drv_twi_t* twi_instance;
static uint32_t range = 375;
static uint32_t resolution = 16;
static float lsb = 1;

static inline void isl29125_reg_read(uint8_t reg, uint8_t* data, uint8_t len) {
  nrf_drv_twi_enable(twi_instance);
  nrf_drv_twi_tx(twi_instance, ISL29125_ADDR, &reg, 1, true);
  nrf_drv_twi_rx(twi_instance, ISL29125_ADDR, data, len);
  nrf_drv_twi_disable(twi_instance);
}

static inline void isl29125_reg_write(uint8_t reg, uint8_t data) {
  uint8_t cmd[2] = {reg, data};

  nrf_drv_twi_enable(twi_instance);
  nrf_drv_twi_tx(twi_instance, ISL29125_ADDR, cmd, 2, true);
  nrf_drv_twi_disable(twi_instance);
}

void isl29125_init(nrf_drv_twi_t* instance) {
  twi_instance = instance;
}
void isl29125_config(isl29125_config_t config) {
  uint8_t config_byte = config.sync_int << 4 |
                        config.resolution << 3 |
                        config.range << 2 |
                        config.mode;
  isl29125_reg_write(ISL29125_CFG1, config_byte);

  if (config.range) range = 10000;
  else range = 375;
  if (config.resolution) resolution = 12;
  else resolution = 16;
  lsb = range / (float)pow(2, resolution);
}
void isl29125_read_lux(float* red, float* green, float* blue) {
  uint8_t data[6];
  isl29125_reg_read(ISL29125_G_LOW, data, 6);
  *green  = (data[0] | data[1] << 8) * lsb;
  *red    = (data[2] | data[3] << 8) * lsb;
  *blue   = (data[4] | data[5] << 8) * lsb;
}

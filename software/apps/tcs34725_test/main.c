#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "nrf_drv_clock.h"
#include "nrf_power.h"

#include "permamote.h"
#include "tcs34725.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

void twi_init(void) {
  ret_code_t err_code;

  const nrf_drv_twi_config_t twi_config = {
    .scl                = I2C_SCL,
    .sda                = I2C_SDA,
    .frequency          = NRF_TWI_FREQ_400K,
  };

  //err_code = nrf_drv_twi_init(&twi_instance, &twi_config, NULL, NULL);
  //APP_ERROR_CHECK(err_code);

  err_code = nrf_twi_mngr_init(&twi_mngr_instance, &twi_config);
  APP_ERROR_CHECK(err_code);
}

int main(void) {
  // init softdevice
  //nrf_sdh_enable_request();
  //sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  nrf_power_dcdcen_set(1);

  // init led
  //led_init(LED2);
  //led_off(LED2);

  printf("\nCOLOR TEST\n");

  // Init twi
  twi_init();

  //timer_init();

  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(TCS34725_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_clear(TCS34725_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  nrf_delay_ms(3);

  tcs34725_config_t config = {
    .int_time = TCS34725_INTEGRATIONTIME_154MS,
    .gain = TCS34725_GAIN_1X,
  };

  tcs34725_init(&twi_mngr_instance);
  tcs34725_config(config);
  tcs34725_enable();

  uint16_t red, green, blue, clear;
  float cct;

  while (1) {
    nrf_delay_ms(5000);
    tcs34725_read_channels(&red, &green, &blue, &clear);
    tcs34725_ir_compensate(&red, &green, &blue, &clear);
    cct = tcs34725_calculate_cct(red, green, blue);
    printf("r: %d, g: %d, b: %d, c: %d\n", red, green, blue, clear);
    printf("cct: %f\n", cct);
    tcs34725_read_channels(&red, &green, &blue, &clear);
  }
}

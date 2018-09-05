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
#include "ms5637.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

void twi_init(void) {
  ret_code_t err_code;

  const nrf_drv_twi_config_t twi_config = {
    .scl                = I2C_SCL,
    .sda                = I2C_SDA,
    .frequency          = NRF_TWI_FREQ_400K,
  };

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

  printf("\nTEMP/PRESSURE TEST\n");

  // Init twi
  twi_init();


  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_clear(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  ms5637_init(&twi_mngr_instance, osr_8192);
  ms5637_start();

  float pressure, temperature;

  while (1) {
    nrf_delay_ms(5000);
    ms5637_get_temperature_and_pressure(&temperature, &pressure);
    //temperature = ms5637_get_temperature();
    printf("temperature: %f, pressure: %f\n", temperature, pressure);
    //__WFE();
  }
}

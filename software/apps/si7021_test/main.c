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
#include "si7021.h"

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

  printf("\nTEMP/HUMIDITY TEST\n");

  // Init twi
  twi_init();


  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_clear(SI7021_EN);

  nrf_delay_ms(20);
  si7021_init(&twi_mngr_instance);
  //si7021_reset();
  si7021_config(si7021_mode0);

  float humidity, temperature = 0;

  while (1) {
    nrf_delay_ms(5000);
    si7021_read_temp_and_RH(&temperature, &humidity);
    printf("humidity: %f, temperature: %f\n", humidity, temperature);
    //__WFE();
  }
}

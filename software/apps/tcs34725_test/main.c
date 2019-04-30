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
#include "app_timer.h"

#include "permamote.h"
#include "tcs34725.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

void timer_init(void)
{
  ret_code_t err_code = nrf_drv_clock_init();
  APP_ERROR_CHECK(err_code);
  nrf_drv_clock_lfclk_request(NULL);

  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
}

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

void color_read_callback(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear) {
  float cct;

  tcs34725_off();
  tcs34725_ir_compensate(&red, &green, &blue, &clear);
  cct = tcs34725_calculate_ct(red, blue);

  printf("\nSensed light cct: %f\n",cct);
  printf("Sensed light color:\n  r: %u\n  g: %u\n  b: %u\n", (uint16_t)red, (uint16_t)green, (uint16_t)blue);
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
  timer_init();

  //timer_init();

  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_cfg_output(PIR_EN);
  nrf_gpio_cfg_output(LED_1);
  nrf_gpio_cfg_output(LED_2);
  nrf_gpio_cfg_output(LED_3);
  nrf_gpio_pin_set(LED_1);
  nrf_gpio_pin_set(LED_2);
  nrf_gpio_pin_set(LED_3);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_set(PIR_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  nrf_gpio_cfg_output(LI2D_CS);
  nrf_gpio_cfg_output(SPI_MISO);
  nrf_gpio_cfg_output(SPI_MOSI);
  nrf_gpio_pin_set(LI2D_CS);
  nrf_gpio_pin_set(SPI_MISO);
  nrf_gpio_pin_set(SPI_MOSI);

  nrf_delay_ms(5);

  tcs34725_init(&twi_mngr_instance);

  while (1) {
    nrf_delay_ms(5000);
    tcs34725_on();
    tcs34725_config_agc();
    tcs34725_enable();
    tcs34725_read_channels_agc(color_read_callback);
  }
}

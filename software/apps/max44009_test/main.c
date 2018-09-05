#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_power.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "permamote.h"
#include "max44009.h"

#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

bool update_thresh = false;
float upper;
float lower;

#define SENSOR_RATE APP_TIMER_TICKS(1000)
APP_TIMER_DEF(sensor_read_timer);

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

static void sensor_read_callback(float lux) {
    upper = lux + lux* 0.10;
    lower = lux - lux* 0.10;
    printf("\n#######\nlux: %f, upper: %f, lower: %f\n", lux, upper, lower);

    update_thresh = true;
}

static void interrupt_callback(void) {
    max44009_schedule_read_lux();
}

//static void timer_init(void)
//{
//    uint32_t err_code = app_timer_init();
//    APP_ERROR_CHECK(err_code);
//    err_code = app_timer_create(&sensor_read_timer, APP_TIMER_MODE_REPEATED, read_timer_callback);
//    APP_ERROR_CHECK(err_code);
//    err_code = app_timer_start(sensor_read_timer, SENSOR_RATE, NULL);
//    APP_ERROR_CHECK(err_code);
//}

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

  printf("\nLUX TEST\n");

  // Init twi
  twi_init();

  //timer_init();

  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_pin_clear(MAX44009_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);


  const max44009_config_t config = {
    .continuous = 0,
    .manual = 0,
    .cdr = 0,
    .int_time = 3,
  };

  nrf_delay_ms(500);

  max44009_init(&twi_mngr_instance);
  max44009_set_read_lux_callback(sensor_read_callback);
  max44009_config(config);
  max44009_schedule_read_lux();
  max44009_set_interrupt_callback(interrupt_callback);
  max44009_enable_interrupt();


  while (1) {
    if (update_thresh) {
      max44009_set_upper_threshold(upper);
      max44009_set_lower_threshold(lower);
      update_thresh = false;
    }
    __WFE();
  }
}

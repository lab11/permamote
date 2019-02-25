#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "app_uart.h"
#include "nrf_drv_clock.h"
#include "nrf_power.h"
#include "nrf_drv_spi.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "permamote.h"
#include "ab1815.h"

static nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);

int main(void) {
  nrf_power_dcdcen_set(1);

  printf("\nRTC TEST\n");

  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  ab1815_control_t config;
  struct timeval tv = {.tv_sec = 1534453449};
  ab1815_time_t  time = unix_to_ab1815(tv);
  //{
  //  .minutes = 8,
  //  .hours = 20,
  //  .date = 6,
  //  .months = 8,
  //  .years = 18,
  //  .weekday = 3,
  //};

  ab1815_init(&spi_instance);
  ab1815_get_config(&config);
  config.auto_rst = 1;
  config.write_rtc = 1;
  ab1815_set_config(config);

  ab1815_set_time(time);

  while (1) {
    nrf_delay_ms(5000);
    ab1815_get_time(&time);
    printf("%d:%02d:%02d, %d/%d/20%02d\n", time.hours, time.minutes, time.seconds, time.months, time.date, time.years);
  }
}

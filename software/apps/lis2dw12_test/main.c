#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_spi.h"
#include "nrf_drv_gpiote.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "app_timer.h"
#include "led.h"
#include "app_uart.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"

#include "permamote.h"
#include "lis2dw12.h"

#define MAX_TEST_DATA_BYTES  (15U)
#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

#define SENSOR_RATE APP_TIMER_TICKS(1000)
APP_TIMER_DEF(sensor_read_timer);
nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(0);

void spi_init(void) {
  ret_code_t err_code;

  nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
  spi_config.sck_pin    = SPI_SCLK;
  spi_config.miso_pin   = SPI_MISO;
  spi_config.mosi_pin   = SPI_MOSI;
  spi_config.frequency  = NRF_DRV_SPI_FREQ_2M;
  spi_config.mode       = NRF_DRV_SPI_MODE_3;
  spi_config.bit_order  = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;

  err_code = nrf_drv_spi_init(&spi_instance, &spi_config, NULL, NULL);
  APP_ERROR_CHECK(err_code);
}

static void read_timer_callback (void* p_context) {

}

static void timer_init(void)
{
    uint32_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&sensor_read_timer, APP_TIMER_MODE_REPEATED, read_timer_callback);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(sensor_read_timer, SENSOR_RATE, NULL);
    APP_ERROR_CHECK(err_code);
}

void uart_error_handle (app_uart_evt_t * p_event) {
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    } else if (p_event->evt_type == APP_UART_FIFO_ERROR) {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}

void uart_init(void) {
  uint32_t err_code;

  const app_uart_comm_params_t comm_params =
  {
    UART_RX,
    UART_TX,
    0,
    0,
    APP_UART_FLOW_CONTROL_DISABLED,
    false,
    UART_BAUDRATE_BAUDRATE_Baud115200
  };
  APP_UART_FIFO_INIT(&comm_params,
                     UART_RX_BUF_SIZE,
                     UART_TX_BUF_SIZE,
                     uart_error_handle,
                     APP_IRQ_PRIORITY_LOW,
                     err_code);
  APP_ERROR_CHECK(err_code);

}

int main(void) {
  // Setup BLE
  nrf_sdh_enable_request();
  sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

  // init uart
  //uart_init();
  //printf("\nACC TEST\n");

  // Init spi
  spi_init();
  nrf_gpio_cfg_output(RTC_CS);
  nrf_gpio_cfg_output(LI2D_CS);
  nrf_gpio_pin_set(LI2D_CS);
  nrf_gpio_pin_set(RTC_CS);

  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  lis2dw12_init(&spi_instance, LI2D_CS);

  lis2dw12_config_t config = {
    .odr = lis2dw12_odr_12_5,
    .mode = lis2dw12_low_power,
    .lp_mode = lis2dw12_lp_2,
    .cs_pullup = 0,
    .bdu = 0,
    .auto_increment = 0,
    .on_demand = 0,
    .bandwidth = lis2dw12_bw_odr_2,
    .fs = lis2dw12_fs_2g,
    .high_pass = 0,
    .low_noise = 1,
  };

  lis2dw12_config(config);

  while (1) {
      ret_code_t err_code = sd_app_evt_wait();
      APP_ERROR_CHECK(err_code);
  }
}

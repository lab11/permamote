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
#include "nrf_drv_clock.h"

#include "permamote.h"
#include "lis2dw12.h"

#define MAX_TEST_DATA_BYTES  (15U)
#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

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

  // init uart
  uart_init();
  printf("\nACC TEST\n");

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

  uint8_t write[2] = {0};
  uint8_t read[2] = {0};

  while (1) {
    write[0] = 0x0F | 0x80;
    //write[1] = 0x90;
    nrf_gpio_pin_clear(LI2D_CS);
    int err = nrf_drv_spi_transfer(&spi_instance, write, 2, read, 2);
    nrf_gpio_pin_set(LI2D_CS);
    printf("write: %x, %x\n", write[0], write[1]);
    //write[0] = 0x1E | 0x80;
    //write[1] = 0x00;
    //nrf_gpio_pin_clear(LI2D_CS);
    //nrf_drv_spi_transfer(&spi_instance, write, 2, read, 2);
    //nrf_gpio_pin_set(LI2D_CS);
    printf("read: %x, %x\n", read[0], read[1]);
    //si7021_read_temp_and_RH(&temp, &hum);
    //printf("temperature: %f\n", temp);
    //printf("humidity: %f\n\n", hum);
    nrf_delay_ms(5000);
  }
}

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

#include "permamote.h"
#include "ab1815.h"

#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

#define LED0 NRF_GPIO_PIN_MAP(0,4)
#define LED1 NRF_GPIO_PIN_MAP(0,5)
#define LED2 NRF_GPIO_PIN_MAP(0,6)

bool update_thresh = false;
float upper;
float lower;

NRF_SPI_MNGR_DEF(spi_mngr_instance, 5, 0);

void uart_error_handle (app_uart_evt_t * p_event) {
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    } else if (p_event->evt_type == APP_UART_FIFO_ERROR) {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}

static void sensor_read_callback() {
    printf("test\n");
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

void spi_init(void) {
  ret_code_t err_code;

  const nrf_drv_spi_config_t spi_config= {
    .sck_pin            = SPI_SCLK,
    .miso_pin           = SPI_MISO,
    .mosi_pin           = SPI_MOSI,
    .frequency          = NRF_DRV_SPI_FREQ_4M,
  };

  err_code = nrf_spi_mngr_init(&spi_mngr_instance, &spi_config);
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

  // init uart
  uart_init();

  printf("\nRTC TEST\n");

  // Init spi
  spi_init();


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

  ab1815_init(&spi_mngr_instance);
  ab1815_get_config(&config);
  config.auto_rst = 1;
  config.write_rtc = 1;
  ab1815_set_config(config);

  ab1815_set_time(time);

  while (1) {
    nrf_delay_ms(5000);
    ab1815_get_time(&time);
    printf("%d:%02d:%02d, %d/%d/20%02d\n", time.hours, time.minutes, time.seconds, time.months, time.date, time.years);
    //struct timeval tv = {
    //  .tv_sec = 1533699875,
    //};
    //time = unix_to_ab1815(tv);
    //printf("%d:%02d:%02d, %d/%d/20%02d\n", time.hours, time.minutes, time.seconds, time.months, time.date, time.years);
    //__WFE();
  }
}

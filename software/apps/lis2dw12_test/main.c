#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_spi_mngr.h"
#include "nrf_drv_gpiote.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "led.h"
#include "app_uart.h"
#include "nrf_drv_clock.h"

#include "permamote.h"
#include "lis2dw12.h"

#define MAX_TEST_DATA_BYTES  (15U)
#define UART_TX_BUF_SIZE     2048
#define UART_RX_BUF_SIZE     2048

#define LED1 NRF_GPIO_PIN_MAP(0,5)

NRF_SPI_MNGR_DEF(spi_instance, 5, 0);

static int16_t x[32], y[32], z[32];

lis2dw12_wakeup_config_t wake_config = {
  .sleep_enable = 1,
  .threshold = 0x05,
  .wake_duration = 3,
  .sleep_duration = 2
};

lis2dw12_config_t config = {
  .odr = lis2dw12_odr_200,
  .mode = lis2dw12_low_power,
  .lp_mode = lis2dw12_lp_1,
  .cs_nopull = 0,
  .bdu = 1,
  .auto_increment = 1,
  .i2c_disable = 1,
  .int_active_low = 0,
  .on_demand = 1,
  .bandwidth = lis2dw12_bw_odr_2,
  .fs = lis2dw12_fs_4g,
  .high_pass = 0,
  .low_noise = 1,
};

void spi_init(void) {
  ret_code_t err_code;

  nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
  spi_config.sck_pin    = SPI_SCLK;
  spi_config.miso_pin   = SPI_MISO;
  spi_config.mosi_pin   = SPI_MOSI;
  spi_config.ss_pin     = LI2D_CS;
  spi_config.frequency  = NRF_DRV_SPI_FREQ_4M;
  spi_config.mode       = NRF_DRV_SPI_MODE_3;
  spi_config.bit_order  = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;

  err_code = nrf_spi_mngr_init(&spi_instance, &spi_config);
  APP_ERROR_CHECK(err_code);
}

static void fifo_read_handler(void) {
  printf("heyo!\n");
  for(int i = 0; i < 32; i++) {
    printf("x: %d, y: %d, z: %d\n", x[i], y[i], z[i]);
  }

  // reset fifo
  lis2dw12_fifo_config_t fifo_config = {
    .mode = lis2dw12_fifo_bypass
  };
  lis2dw12_fifo_config(fifo_config);
  fifo_config.mode = lis2dw12_fifo_byp_to_cont;
  lis2dw12_fifo_config(fifo_config);
}

static void acc_wakeup_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  lis2dw12_read_full_fifo(x, y, z, fifo_read_handler);
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

  // init led
  led_init(LED1);
  led_off(LED1);

  // init uart
  uart_init();
  printf("\nACC TEST\n");

  // Init spi
  spi_init();
  nrf_gpio_cfg_output(RTC_CS);
  nrf_gpio_cfg_output(LI2D_CS);
  nrf_gpio_pin_set(LI2D_CS);
  nrf_gpio_pin_set(RTC_CS);

  // Init gpiote, accelerometer interrupt
  nrf_drv_gpiote_init();
  nrf_drv_gpiote_in_config_t int_gpio_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(0);
  int_gpio_config.pull = NRF_GPIO_PIN_NOPULL;
  nrf_drv_gpiote_in_init(LI2D_INT2, &int_gpio_config, acc_wakeup_handler);
  nrf_drv_gpiote_in_event_enable(LI2D_INT2, 1);

  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  lis2dw12_init(&spi_instance);

  lis2dw12_int_config_t int_config = {0};
  int_config.int1_wakeup = 1;
  int_config.int2_fifo_full = 1;

  lis2dw12_reset();
  lis2dw12_config(config);
  lis2dw12_interrupt_config(int_config);
  lis2dw12_interrupt_enable(1);

  // clear fifo
  lis2dw12_fifo_config_t fifo_config;
  fifo_config.mode = lis2dw12_fifo_bypass;
  lis2dw12_fifo_config(fifo_config);
  fifo_config.mode = lis2dw12_fifo_byp_to_cont;
  lis2dw12_fifo_config(fifo_config);

  lis2dw12_wakeup_config(wake_config);

  while (1) {
      ret_code_t err_code = sd_app_evt_wait();
      APP_ERROR_CHECK(err_code);
  }
}

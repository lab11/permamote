/*
 * Send an advertisement periodically
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_twi.h"
#include "nrf_drv_gpiote.h"
#include "app_util_platform.h"
#include "ble_advdata.h"
#include "nordic_common.h"
#include "softdevice_handler.h"
#include "ble_debug_assert_handler.h"
#include "led.h"
#include "app_uart.h"

#include "simple_ble.h"
#include "simple_adv.h"
#include "permamote.h"
#include "ms5637.h"

#define MAX_TEST_DATA_BYTES  (15U)
#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

char name[10] = "Permamote";

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
  .platform_id       = 0x00,              // used as 4th octect in device BLE address
  .device_id         = DEVICE_ID_DEFAULT,
  .adv_name          = name,
  .adv_interval      = MSEC_TO_UNITS(500, UNIT_0_625_MS),
  .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),
  .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS)
};

nrf_drv_twi_t twi_instance = NRF_DRV_TWI_INSTANCE(1);

void twi_init(void) {
  ret_code_t err_code;

  const nrf_drv_twi_config_t twi_config = {
    .scl                = I2C_SCL,
    .sda                = I2C_SDA,
    .frequency          = NRF_TWI_FREQ_100K,
    .interrupt_priority = APP_IRQ_PRIORITY_HIGH
  };

  err_code = nrf_drv_twi_init(&twi_instance, &twi_config, NULL, NULL);
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
    11,
    12,
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
  simple_ble_init(&ble_config);

  // init uart
  uart_init();
  printf("\nPRESSURE AND TEMPERATURE TEST\n");

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

  ms5637_init(&twi_instance);
  ms5637_start();

  // Advertise because why not
  simple_adv_only_name();

  while (1) {
    float temp = ms5637_get_temperature(osr_8192);
    printf("temperature: %f\n", temp);
    float press = ms5637_get_pressure(osr_8192);
    printf("pressure: %f\n\n", press);
    nrf_delay_ms(5000);
  }
}

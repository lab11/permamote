#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
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
#include "max44009.h"

#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

#define LED0 NRF_GPIO_PIN_MAP(0,4)
#define LED1 NRF_GPIO_PIN_MAP(0,5)
#define LED2 NRF_GPIO_PIN_MAP(0,6)

#define SENSOR_RATE APP_TIMER_TICKS(1000)
APP_TIMER_DEF(sensor_read_timer);

uint8_t enables[7] = {
   MAX44009_EN,
   ISL29125_EN,
   MS5637_EN,
   SI7021_EN,
   PIR_EN,
   I2C_SDA,
   I2C_SCL
};

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

void uart_error_handle (app_uart_evt_t * p_event) {
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    } else if (p_event->evt_type == APP_UART_FIFO_ERROR) {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}
static void sensor_read_callback(float lux) {
    printf("lux: %f\n", lux);
}

static void read_timer_callback (void* p_context) {
    max44009_schedule_read_lux();
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
  nrf_sdh_enable_request();
  sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

  // init led
  led_init(LED2);
  led_off(LED2);

  // init uart
  uart_init();

  printf("\nLUX TEST\n");

  // Init twi
  twi_init();

  timer_init();

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

  max44009_init(&twi_mngr_instance, sensor_read_callback);
  max44009_config(config);

  while (1) {
    sd_app_evt_wait();
  }
}

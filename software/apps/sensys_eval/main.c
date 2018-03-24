#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "app_timer.h"
#include "led.h"
#include "app_uart.h"
#include "nrf_drv_clock.h"

#include "permamote.h"
#include "isl29125.h"
#include "max44009.h"

#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

#define LED0 NRF_GPIO_PIN_MAP(0,4)
#define LED1 NRF_GPIO_PIN_MAP(0,5)
#define LED2 NRF_GPIO_PIN_MAP(0,6)

#define SENSOR_RATE APP_TIMER_TICKS(5000)
APP_TIMER_DEF(sensor_read_timer);

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

static uint8_t integrate_count = 0;

void uart_error_handle (app_uart_evt_t * p_event) {
  if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
    APP_ERROR_HANDLER(p_event->data.error_communication);
  } else if (p_event->evt_type == APP_UART_FIFO_ERROR) {
    APP_ERROR_HANDLER(p_event->data.error_code);
  }
}

static void lux_sensor_read_callback(float lux) {
  //printf("lux: %f\n", lux);
}

static void color_sensor_int_handler(void) {
  //nrf_delay_ms(300);
  if (integrate_count < 2) {
    integrate_count++;
    return;
  }

  // reset count and read light
  integrate_count = 0;
  isl29125_schedule_color_read();
}

static void color_sensor_read_callback(uint16_t red, uint16_t green, uint16_t blue) {
  //isl29125_power_down();
  //isl29125_interrupt_disable();
  nrf_gpio_pin_set(ISL29125_EN);
  //printf("red: %u, green: %u, blue %u\n", red, green, blue);
}

static void read_timer_callback (void* p_context) {
  // turn on and configure color sensor
  nrf_gpio_pin_clear(ISL29125_EN);
  isl29125_interrupt_enable();
  isl29125_power_on();

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
  //uart_init();

  //printf("\nLIGHT + COLOR TEST\n");

  // Init twi
  twi_init();

  timer_init();

  // power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  nrf_gpio_pin_clear(MAX44009_EN);
  const max44009_config_t lux_config = {
    .continuous = 0,
    .manual = 0,
    .cdr = 0,
    .int_time = 3,
  };
  max44009_init(&twi_mngr_instance, lux_sensor_read_callback);
  max44009_config(lux_config);

  const isl29125_config_t color_config = {
    .mode = isl29125_mode_green_red_blue,
    .resolution = 1,
  };
  const isl29125_int_config_t color_int_config = {
    .int_conversion_done = 1,
  };
  isl29125_init(&twi_mngr_instance, color_config, color_int_config, color_sensor_read_callback, color_sensor_int_handler);

  while (1) {
    sd_app_evt_wait();
  }
}

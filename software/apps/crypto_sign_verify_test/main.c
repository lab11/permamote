#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "led.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "app_uart.h"
#include "nrf_crypto.h"

#include "permamote.h"

#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

#define DATA_SIZE 1024
#define LED0 NRF_GPIO_PIN_MAP(0,4)
#define LED1 NRF_GPIO_PIN_MAP(0,5)
#define LED2 NRF_GPIO_PIN_MAP(0,6)

uint8_t data_buffer[DATA_SIZE];

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

uint8_t enables[5] = {
   MAX44009_EN,
   ISL29125_EN,
   MS5637_EN,
   SI7021_EN,
   PIR_EN,
};

int main(void) {

    // Initialize.
    led_init(LED0);
    led_init(LED1);
    led_init(LED2);
    led_off(LED0);
    led_off(LED1);
    led_off(LED2);

    nrf_sdh_enable_request();
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

    for (int i = 0; i < 5; i++) {
      nrf_gpio_cfg_output(enables[i]);
      nrf_gpio_pin_set(enables[i]);
    }

    nrf_gpio_cfg_output(LI2D_CS);
    nrf_gpio_cfg_output(SPI_MISO);
    nrf_gpio_cfg_output(SPI_MOSI);
    nrf_gpio_pin_set(LI2D_CS);
    nrf_gpio_pin_set(SPI_MISO);
    nrf_gpio_pin_set(SPI_MOSI);

    // use SCL for debug output
    //nrf_gpio_cfg_output(I2C_SCL);
    //nrf_gpio_pin_clear(I2C_SCL);

    // init uart
    uart_init();
    printf("CRYPTO!");

    // generate a pseudo-random buffer
    //srand(42);
    //for (int i = 0; i < 5; i++) {
    //  data_buffer[i] = (rand()%256);
    //  printf("%d", data_buffer[i]);
    //}

    //nrf_crypto_init();

    // Enter main loop.
    while (1) {
      //nrf_delay_ms(500);
      //led_toggle(LED);
      ret_code_t err_code = sd_app_evt_wait();
      APP_ERROR_CHECK(err_code);
    }
}

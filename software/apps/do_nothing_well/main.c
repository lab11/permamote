/* Blink
 */

#include <stdbool.h>
#include <stdint.h>
#include "led.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"

#include "permamote.h"

#define LED0 NRF_GPIO_PIN_MAP(0,4)
#define LED1 NRF_GPIO_PIN_MAP(0,5)
#define LED2 NRF_GPIO_PIN_MAP(0,6)

uint8_t enables[7] = {
   MAX44009_EN,
   ISL29125_EN,
   MS5637_EN,
   SI7021_EN,
   PIR_EN,
   I2C_SDA,
   I2C_SCL
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

    for (int i = 0; i < 7; i++) {
      nrf_gpio_cfg_output(enables[i]);
      nrf_gpio_pin_set(enables[i]);
    }

    nrf_gpio_cfg_output(LI2D_CS);
    nrf_gpio_cfg_output(SPI_MISO);
    nrf_gpio_cfg_output(SPI_MOSI);
    nrf_gpio_pin_set(LI2D_CS);
    nrf_gpio_pin_set(SPI_MISO);
    nrf_gpio_pin_set(SPI_MOSI);

    // Enter main loop.
    while (1) {
      //nrf_delay_ms(500);
      //led_toggle(LED);
      ret_code_t err_code = sd_app_evt_wait();
      APP_ERROR_CHECK(err_code);
    }
}

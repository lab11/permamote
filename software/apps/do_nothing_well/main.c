/* Blink
 */

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "nrf_gpio.h"
#include "nrf_power.h"

#include "permamote.h"

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
#ifdef SOFTDEVICE_PRESENT
    nrf_sdh_enable_request();
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
#else
    nrf_power_dcdcen_set(1);
#endif

    nrf_gpio_cfg_output(LED_1);
    nrf_gpio_cfg_output(LED_2);
    nrf_gpio_cfg_output(LED_3);
    nrf_gpio_pin_set(LED_1);
    nrf_gpio_pin_set(LED_2);
    nrf_gpio_pin_set(LED_3);
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
#ifdef SOFTDEVICE_PRESENT
      ret_code_t err_code = sd_app_evt_wait();
      APP_ERROR_CHECK(err_code);
#endif
      __WFE();

    }
}

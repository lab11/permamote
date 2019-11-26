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
#include "nrf_twi_mngr.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "permacam.h"

#include "hm01b0.h"
#include "HM01B0_Walking1s_01.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

uint8_t enables[7] = {
   HM01B0_ENn,
   MAX44009_EN,
   PIR_EN,
   I2C_SDA,
   I2C_SCL
};

void twi_init(const nrf_twi_mngr_t * twi_instance) {
  ret_code_t err_code;

  const nrf_drv_twi_config_t twi_config = {
    .scl                = I2C_SCL,
    .sda                = I2C_SDA,
    .frequency          = NRF_TWI_FREQ_400K,
  };

  err_code = nrf_twi_mngr_init(twi_instance, &twi_config);
  APP_ERROR_CHECK(err_code);
}

void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void) {

    // Initialize.
    nrf_power_dcdcen_set(1);
    log_init();

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

    nrf_gpio_cfg_output(SPI_MISO);
    nrf_gpio_cfg_output(SPI_MOSI);
    nrf_gpio_pin_set(SPI_MISO);
    nrf_gpio_pin_set(SPI_MOSI);

    twi_init(&twi_mngr_instance);

    hm01b0_init_if(&twi_mngr_instance);

    NRF_LOG_INFO("turning on camera");

    hm01b0_power_up();
    hm01b0_mclk_enable();
    uint16_t model_id = 0xFF;
    int error = hm01b0_get_modelid(&model_id);
    NRF_LOG_INFO("error: %d, model id: 0x%x", error, model_id);
    error = hm01b0_init_system(sHM01b0TestModeScript_Walking1s, sizeof(sHM01b0TestModeScript_Walking1s));
    NRF_LOG_INFO("error: %d", error);

    // Enter main loop.
    while (1) {
        //nrf_delay_ms(500);
        //led_toggle(LED);
        if (NRF_LOG_PROCESS() == false)
        {
            __WFE();
        }

    }
}

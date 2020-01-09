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
#include "nrf_drv_spi.h"

#include "permacam.h"

#include "hm01b0.h"
#include "ab1815.h"
#include "HM01B0_SERIAL_FULL_8bits_msb_5fps.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);
static nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);

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
    uint8_t image_buffer[HM01B0_IMAGE_SIZE];


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

    twi_init(&twi_mngr_instance);

    // setup RTC
    ab1815_init(&spi_instance);
    ab1815_control_t ab1815_config;
    ab1815_get_config(&ab1815_config);
    ab1815_config.auto_rst = 1;
    ab1815_set_config(ab1815_config);

    NRF_LOG_INFO("turning on camera");
    NRF_LOG_INFO("address of buffer: %x", image_buffer);
    NRF_LOG_INFO("size of buffer:    %x", sizeof(image_buffer));

    hm01b0_init_if(&twi_mngr_instance);
    hm01b0_mclk_init();

    nrf_gpio_pin_clear(LED_1);
    hm01b0_power_up();

    int error = hm01b0_init_system(sHM01B0InitScript, sizeof(sHM01B0InitScript)/sizeof(hm_script_t));
    NRF_LOG_INFO("error: %d", error);

    hm01b0_blocking_read_oneframe(image_buffer, sizeof(image_buffer));
    nrf_gpio_pin_set(LED_1);
    hm01b0_power_down();

    // Enter main loop.
    while (1) {
        //led_toggle(LED);
        if (NRF_LOG_PROCESS() == false)
        {
            __WFI();
        }
    }
}

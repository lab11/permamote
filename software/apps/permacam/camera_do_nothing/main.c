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

uint8_t enables[8] = {
   HM01B0_ENn,
   MAX44009_EN,
   PIR_EN,
   LED_1,
   LED_2,
   LED_3
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

    NRF_LOG_INFO("HO HO");

    for (int i = 0; i < 8; i++) {
      nrf_gpio_cfg_output(enables[i]);
      nrf_gpio_pin_set(enables[i]);
    }
    nrf_gpio_cfg_output(HM01B0_MCLK);
    nrf_gpio_cfg_output(HM01B0_PCLKO);
    nrf_gpio_cfg_output(HM01B0_FVLD);
    nrf_gpio_cfg_output(HM01B0_LVLD);
    nrf_gpio_cfg_output(HM01B0_CAM_D0);
    nrf_gpio_cfg_output(HM01B0_CAM_TRIG);
    nrf_gpio_cfg_output(HM01B0_CAM_INT);
    nrf_gpio_cfg_output(PIR_OUT);
    nrf_gpio_cfg_output(I2C_SDA);
    nrf_gpio_cfg_output(I2C_SCL);
    nrf_gpio_cfg_output(SPI_SCLK);
    nrf_gpio_cfg_output(SPI_MISO);
    nrf_gpio_cfg_output(SPI_MOSI);
    nrf_gpio_cfg_output(RTC_CS);
    nrf_gpio_cfg_output(RTC_WDI);

    nrf_gpio_pin_clear(HM01B0_MCLK);
    nrf_gpio_pin_clear(HM01B0_PCLKO);
    nrf_gpio_pin_clear(HM01B0_FVLD);
    nrf_gpio_pin_clear(HM01B0_LVLD);
    nrf_gpio_pin_clear(HM01B0_CAM_D0);
    nrf_gpio_pin_clear(HM01B0_CAM_TRIG);
    nrf_gpio_pin_clear(HM01B0_CAM_INT);
    nrf_gpio_pin_clear(PIR_OUT);
    nrf_gpio_pin_set(I2C_SDA);
    nrf_gpio_pin_set(I2C_SCL);
    nrf_gpio_pin_clear(SPI_SCLK);
    nrf_gpio_pin_clear(SPI_MISO);
    nrf_gpio_pin_clear(SPI_MOSI);
    nrf_gpio_pin_set(RTC_CS);
    nrf_gpio_pin_set(RTC_WDI);

    nrf_delay_ms(5);

    // setup RTC
    ab1815_init(&spi_instance);
    uint8_t status = 0;
    ab1815_control_t ab1815_config;
    ab1815_get_config(&ab1815_config);
    ab1815_config.auto_rst = 1;
    ab1815_set_config(ab1815_config);


    // Enter main loop.
    while (1) {
        //led_toggle(LED);
        if (NRF_LOG_PROCESS() == false)
        {
            __WFI();
        }
    }
}

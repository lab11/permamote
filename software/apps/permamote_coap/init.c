#include "init.h"
#include "callbacks.h"
#include "config.h"

#include "nrf_drv_saadc.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"

#include "permamote.h"
#include "ab1815.h"
#include "max44009.h"
#include "ms5637.h"
#include "si7021.h"
#include "tcs34725.h"

int sensors_init(const nrf_twi_mngr_t* twi_mngr_instance, const nrf_drv_spi_t* spi_instance){
  // Set power gates
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_cfg_output(PIR_EN);
  nrf_gpio_cfg_output(LED_1);
  nrf_gpio_cfg_output(LED_2);
  nrf_gpio_cfg_output(LED_3);
  nrf_gpio_pin_set(LED_1);
  nrf_gpio_pin_set(LED_2);
  nrf_gpio_pin_set(LED_3);
  nrf_gpio_pin_clear(MAX44009_EN);
  nrf_gpio_pin_clear(PIR_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  nrf_gpio_cfg_output(LI2D_CS);
  nrf_gpio_cfg_output(SPI_MISO);
  nrf_gpio_cfg_output(SPI_MOSI);
  nrf_gpio_pin_set(LI2D_CS);
  nrf_gpio_pin_set(SPI_MISO);
  nrf_gpio_pin_set(SPI_MOSI);

  nrf_delay_ms(5);

  // setup interrupt for pir
  if (!nrf_drv_gpiote_is_init()) {
    nrf_drv_gpiote_init();
  }
  nrf_drv_gpiote_in_config_t pir_gpio_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(0);
  pir_gpio_config.pull = NRF_GPIO_PIN_PULLDOWN;
  nrf_drv_gpiote_in_init(PIR_OUT, &pir_gpio_config, pir_interrupt_callback);

  // setup vbat sense
  nrf_gpio_cfg_input(VBAT_OK, NRF_GPIO_PIN_NOPULL);

  // setup RTC
  ab1815_init(spi_instance);
  ab1815_control_t ab1815_config;
  ab1815_get_config(&ab1815_config);
  ab1815_config.auto_rst = 1;
  ab1815_set_config(ab1815_config);
  ab1815_time_t alarm_time = {.hours = 8};
  ab1815_set_alarm(alarm_time, ONCE_PER_HOUR, (ab1815_alarm_callback*) rtc_callback);
  ab1815_clear_watchdog();
  ab1815_set_watchdog(1, 18, _1_4HZ);


  // init sensors
  ms5637_init(twi_mngr_instance, osr_4096);

  si7021_init(twi_mngr_instance);

  max44009_init(twi_mngr_instance);

  tcs34725_init(twi_mngr_instance);

  // setup light sensor
  const max44009_config_t config = {
    .continuous = 0,
    .manual = 0,
    .cdr = 0,
    .int_time = 10,
  };

  max44009_set_read_lux_callback(light_sensor_read_callback);
  max44009_set_interrupt_callback(light_interrupt_callback);
  max44009_config(config);

  return 0;
}

void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

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

void saadc_handler(nrf_drv_saadc_evt_t const * p_event) {
}

void adc_init(void) {
  // set up voltage ADC
  nrf_saadc_channel_config_t primary_channel_config =
    NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN5);
  primary_channel_config.burst = NRF_SAADC_BURST_ENABLED;

  nrf_saadc_channel_config_t solar_channel_config =
    NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN6);

  nrf_saadc_channel_config_t secondary_channel_config =
    NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN7);

  nrf_drv_saadc_init(NULL, saadc_handler);

  nrf_drv_saadc_channel_init(0, &primary_channel_config);
  nrf_drv_saadc_channel_init(1, &solar_channel_config);
  nrf_drv_saadc_channel_init(2, &secondary_channel_config);
  nrf_drv_saadc_calibrate_offset();
}

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_util.h"
#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "app_timer.h"
#include "led.h"
#include "app_uart.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_saadc.h"

#include "simple_ble.h"
#include "simple_adv.h"
#include "multi_adv.h"
#include "eddystone.h"
#include "permamote.h"
#include "isl29125.h"
#include "max44009.h"

#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

#define LED0 NRF_GPIO_PIN_MAP(0,4)
#define LED1 NRF_GPIO_PIN_MAP(0,5)
#define LED2 NRF_GPIO_PIN_MAP(0,6)

#define APP_ADV_INTERVAL  MSEC_TO_UNITS(500, UNIT_0_625_MS)
#define MIN_CONN_INTERVAL MSEC_TO_UNITS(500, UNIT_1_25_MS)
#define MAX_CONN_INTERVAL MSEC_TO_UNITS(1000, UNIT_1_25_MS)

#define UMICH_COMPANY_IDENTIFIER      0x02E0
#define APP_BEACON_INFO_LENGTH        19
#define APP_BEACON_INFO_SERVICE_PERM  0x20 // Registered to Permamote

// Maximum size is 17 characters, counting URLEND if used
// Demo App (using j2x and cdn.rawgit.com)
#define PHYSWEB_URL     "j2x.us/perm"
#define ADV_SWITCH_MS 500

#define SENSOR_RATE APP_TIMER_TICKS(1000)
APP_TIMER_DEF(sensor_read_timer);

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH];
static bool data_updated = false;

static struct {
    float light;
    uint8_t   light_exp;
    uint8_t   light_mant;
    uint16_t  color_red;
    uint16_t  color_green;
    uint16_t  color_blue;
    int16_t   vbat;
    int16_t   vsol;
    int16_t   vsec;
    uint32_t  sequence_num;
} m_sensor_info = {0};

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
    .platform_id       = 0x11,              // used as 4th octect in device BLE address
    .device_id         = DEVICE_ID_DEFAULT,
    .adv_name          = DEVICE_NAME,       // used in advertisements if there is room
    .adv_interval      = APP_ADV_INTERVAL,
    .min_conn_interval = MIN_CONN_INTERVAL,
    .max_conn_interval = MAX_CONN_INTERVAL,
};

static uint8_t integrate_count = 0;

void uart_error_handle (app_uart_evt_t * p_event) {
  if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
    APP_ERROR_HANDLER(p_event->data.error_communication);
  } else if (p_event->evt_type == APP_UART_FIFO_ERROR) {
    APP_ERROR_HANDLER(p_event->data.error_code);
  }
}

void saadc_handler(nrf_drv_saadc_evt_t * p_event) {
}

static void lux_sensor_read_callback(float lux, uint8_t mant, uint8_t exp) {
  //printf("lux: %f\n", lux);
  m_sensor_info.light = lux;
  m_sensor_info.light_mant = mant;
  m_sensor_info.light_exp = exp;
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
  nrf_gpio_pin_set(ISL29125_EN);
  //printf("red: %u, green: %u, blue %u\n", red, green, blue);
  m_sensor_info.color_red = red;
  m_sensor_info.color_green = green;
  m_sensor_info.color_blue = blue;

  data_updated = true;
}

static void read_timer_callback (void* p_context) {
  // turn on and configure color sensor
  nrf_gpio_pin_clear(ISL29125_EN);
  isl29125_interrupt_enable();
  isl29125_power_on();

  max44009_schedule_read_lux();

  nrf_saadc_value_t adc_samples[3];
  nrf_drv_saadc_sample_convert(0, adc_samples);
  nrf_drv_saadc_sample_convert(1, adc_samples+1);
  nrf_drv_saadc_sample_convert(2, adc_samples+2);

  m_sensor_info.vbat = adc_samples[0];
  m_sensor_info.vsol = adc_samples[1];
  m_sensor_info.vsec = adc_samples[2];

  //m_sensor_info.vbat = 0.6 * 6 * (float)adc_samples[0] / ((1 << 10)-1);
  //m_sensor_info.vsol = 0.6 * 6 * (float)adc_samples[1] / ((1 << 10)-1);
  //m_sensor_info.vsec = 0.6 * 6 * (float)adc_samples[1] / ((1 << 10)-1);

  //printf("vbat: %.02f, raw = 0x%x\n", m_sensor_info.vbat, adc_samples[0]);
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

/*******************************************************************************
 *   Configure Advertisements
 ******************************************************************************/

static void adv_config_eddystone () {
    eddystone_adv(PHYSWEB_URL, NULL);
}

static void adv_config_data () {
    ble_advdata_manuf_data_t manuf_specific_data;

    // update sequence number only if data actually changed
    if (data_updated) {
        m_sensor_info.sequence_num++;
        data_updated = false;
    }

    // Register this manufacturer data specific data as the BLEES service
    m_beacon_info[0] = APP_BEACON_INFO_SERVICE_PERM;
    //memcpy(&m_beacon_info[1],  &m_sensor_info.light, 4);
    memcpy(&m_beacon_info[1],  &m_sensor_info.light_exp, 1);
    memcpy(&m_beacon_info[2],  &m_sensor_info.light_mant, 1);
    memcpy(&m_beacon_info[3],  &m_sensor_info.color_red, 2);
    memcpy(&m_beacon_info[5],  &m_sensor_info.color_green, 2);
    memcpy(&m_beacon_info[7],  &m_sensor_info.color_blue, 2);
    memcpy(&m_beacon_info[9],  &m_sensor_info.vbat, 2);
    memcpy(&m_beacon_info[11],  &m_sensor_info.vsol, 2);
    memcpy(&m_beacon_info[13],  &m_sensor_info.vsec, 2);
    memcpy(&m_beacon_info[15],  &m_sensor_info.sequence_num, 4);

    memset(&manuf_specific_data, 0, sizeof(manuf_specific_data));
    manuf_specific_data.company_identifier = UMICH_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data = (uint8_t*) m_beacon_info;
    manuf_specific_data.data.size   = APP_BEACON_INFO_LENGTH;

    simple_adv_manuf_data(&manuf_specific_data);
}

/*******************************************************************************
 *   MAIN LOOP
 ******************************************************************************/

int main(void) {
  // init uart
  //uart_init();

  // init softdevice
  //nrf_sdh_enable_request();
  //sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  simple_ble_init(&ble_config);

  multi_adv_init(ADV_SWITCH_MS);
  // Now register our advertisement configure functions
  multi_adv_register_config(adv_config_eddystone);
  multi_adv_register_config(adv_config_data);

  // init led
  led_init(LED2);
  led_off(LED2);


  //printf("\r\nLIGHT + COLOR TEST\r\n");

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

  // set up light sensors
  nrf_gpio_pin_clear(MAX44009_EN);
  const max44009_config_t lux_config = {
    .continuous = 0,
    .manual = 0,
    .cdr = 0,
    .int_time = 0,
  };
  max44009_init(&twi_mngr_instance, lux_sensor_read_callback);
  max44009_config(lux_config);

  const isl29125_config_t color_config = {
    .range = 1,
    .mode = isl29125_mode_green_red_blue,
    .resolution = 0,
  };
  const isl29125_int_config_t color_int_config = {
    .int_conversion_done = 1,
  };
  isl29125_init(&twi_mngr_instance, color_config, color_int_config, color_sensor_read_callback, color_sensor_int_handler);

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
  nrf_delay_ms(500);

  multi_adv_start();

  while (1) {
    power_manage();
  }
}

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_gpiote.h"
#include "nrf_power.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include <openthread/openthread.h>

#include "simple_thread.h"
#include "thread_coap.h"
#include "device_id.h"

#include "permamote.h"
#include "max44009.h"

#define LED0 NRF_GPIO_PIN_MAP(0,4)
#define LED1 NRF_GPIO_PIN_MAP(0,5)
#define LED2 NRF_GPIO_PIN_MAP(0,6)

#define COAP_SERVER_ADDR "64:ff9b::22da:2eb5"

#define DEFAULT_CHILD_TIMEOUT    40                                         /**< Thread child timeout [s]. */
#define DEFAULT_POLL_PERIOD      1000                                       /**< Thread Sleepy End Device polling period when MQTT-SN Asleep. [ms] */
#define NUM_SLAAC_ADDRESSES      4                                          /**< Number of SLAAC addresses. */

static uint8_t device_id[6];
static otIp6Address m_peer_address =
{
    .mFields =
    {
        .m8 = {0}
    }
};

static bool update_thresh = false;
static float upper;
static float lower;

#define DISCOVER_PERIOD     APP_TIMER_TICKS(5*60*1000)
#define PIR_BACKOFF_PERIOD  APP_TIMER_TICKS(5*1000)
APP_TIMER_DEF(discover_send_timer);
APP_TIMER_DEF(pir_backoff);

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

static void discover_send_callback() {
  char* addr = "j2x.us/perm";
  uint8_t data[2 + 6 + strlen(addr)];
  data[0] = 6;
  memcpy(data+1, device_id, 6);
  data[7] = strlen(addr);
  memcpy(data+1+6+1, addr, strlen(addr));

    thread_coap_send(thread_get_instance(), OT_COAP_CODE_PUT, OT_COAP_TYPE_NON_CONFIRMABLE, &m_peer_address, "discovery", data, 2 + 6 + strlen(addr));
}

static void sensor_read_callback(float lux) {
  upper = lux + lux* 0.1;
  lower = lux - lux* 0.1;
  NRF_LOG_INFO("Sensed: lux: %u, upper: %u, lower: %u", (uint32_t)lux, (uint32_t)upper, (uint32_t)lower);

  update_thresh = true;

  uint8_t data[1 + 6 + sizeof(float)];
  data[0] = 6;
  memcpy(data+1, device_id, 6);
  memcpy(data+1+6, &lux, sizeof(float));

  thread_coap_send(thread_get_instance(), OT_COAP_CODE_PUT, OT_COAP_TYPE_NON_CONFIRMABLE, &m_peer_address, "light_lux", data, 1+6+sizeof(float));
}

static void pir_backoff_callback() {
  nrf_drv_gpiote_in_event_enable(PIR_OUT, 1);
}

static void pir_interrupt_callback(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  nrf_drv_gpiote_in_event_disable(PIR_OUT);
  NRF_LOG_INFO("Saw motion");
  uint32_t err_code = app_timer_start(pir_backoff, PIR_BACKOFF_PERIOD, NULL);
  APP_ERROR_CHECK(err_code);
  uint8_t data[2+6];
  data[0] = 6;
  memcpy(data+1, device_id, 6);
  data[7] = 1;
  thread_coap_send(thread_get_instance(), OT_COAP_CODE_PUT, OT_COAP_TYPE_NON_CONFIRMABLE, &m_peer_address, "motion", data, 8);
}

static void light_interrupt_callback(void) {
    max44009_schedule_read_lux();
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void timer_init(void)
{
  uint32_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_create(&discover_send_timer, APP_TIMER_MODE_REPEATED, discover_send_callback);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_create(&pir_backoff, APP_TIMER_MODE_SINGLE_SHOT, pir_backoff_callback);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_start(discover_send_timer, DISCOVER_PERIOD, NULL);
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
  //nrf_sdh_enable_request();
  //sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  nrf_power_dcdcen_set(1);

  // Init log
  log_init();

  // Init twi
  twi_init();

  // init periodic timers
  timer_init();

  NRF_LOG_INFO("\nLUX TEST\n");

  get_device_id(device_id);

  // setup thread
  otIp6AddressFromString(COAP_SERVER_ADDR, &m_peer_address);
  thread_config_t thread_config = {
    .channel = 11,
    .panid = 0xFACE,
    .sed = true,
    .poll_period = DEFAULT_POLL_PERIOD,
    .child_period = DEFAULT_CHILD_TIMEOUT,
    .autocommission = true,
  };

  // Turn on power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_cfg_output(PIR_EN);
  nrf_gpio_pin_clear(MAX44009_EN);
  nrf_gpio_pin_clear(PIR_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);

  // setup light sensor
  const max44009_config_t config = {
    .continuous = 0,
    .manual = 0,
    .cdr = 0,
    .int_time = 3,
  };

  // setup interrupt for pir
  if (!nrf_drv_gpiote_is_init()) {
    nrf_drv_gpiote_init();
  }

  if (!nrf_drv_gpiote_is_init()) {
    nrf_drv_gpiote_init();
  }
  nrf_drv_gpiote_in_config_t pir_gpio_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(1);
  pir_gpio_config.pull = NRF_GPIO_PIN_PULLDOWN;
  int error = nrf_drv_gpiote_in_init(PIR_OUT, &pir_gpio_config, pir_interrupt_callback);
  nrf_drv_gpiote_in_event_enable(PIR_OUT, 1);

  max44009_init(&twi_mngr_instance);
  max44009_set_read_lux_callback(sensor_read_callback);
  max44009_config(config);
  max44009_schedule_read_lux();
  max44009_set_interrupt_callback(light_interrupt_callback);
  max44009_enable_interrupt();


  thread_init(&thread_config);
  otInstance* thread_instance = thread_get_instance();
  thread_coap_client_init(thread_instance);

  while (1) {
    thread_process();
    if (update_thresh) {
      max44009_set_upper_threshold(upper);
      max44009_set_lower_threshold(lower);
      update_thresh = false;
    }
    if (NRF_LOG_PROCESS() == false)
    {
      thread_sleep();
    }
  }
}

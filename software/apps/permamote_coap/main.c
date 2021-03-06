#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "app_timer.h"
#include "mem_manager.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_saadc.h"
#include "nrf_power.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_dfu_utils.h"
#include "nrf_dfu_settings.h"
#include "coap_dfu.h"
#include "coap_api.h"
#include "background_dfu_state.h"
#include "fds.h"

#include <openthread/message.h>

#include "simple_thread.h"
#include "thread_ntp.h"
#include "gateway_coap.h"
#include "thread_coap.h"
#include "thread_dns.h"
#include "device_id.h"
#include "flash_storage.h"

#include "permamote.h"
#include "parse.pb.h"
#include "ab1815.h"
#include "max44009.h"
#include "ms5637.h"
#include "si7021.h"
#include "tcs34725.h"

#include "init.h"
#include "callbacks.h"
#include "config.h"

/*
 * ntp and coap endpoint addresses, to be populated by DNS
 * ========================================================
 * */
static otIp6Address unspecified_ipv6;
static otIp6Address m_ntp_address;
static otIp6Address m_coap_address;
static otIp6Address m_up_address;
#define NUM_ADDRESSES 3
static otIp6Address* addresses[NUM_ADDRESSES] = {&m_ntp_address, &m_coap_address, &m_up_address};
static bool addr_fail[NUM_ADDRESSES] = {0};

/*
 * Permamote specific declarations for packet structure, state machine
 * ==========================
 * */
typedef struct {
  uint8_t voltage;
  uint8_t temp_pres_hum;
  uint8_t color;
  uint8_t discover;
} permamote_sensor_period_t;
uint32_t period_count = 0;
uint32_t hours_count = 0;

static permamote_sensor_period_t sensor_period = {
  .voltage = VOLTAGE_PERIOD,
  .temp_pres_hum = TPH_PERIOD,
  .color = COLOR_PERIOD,
  .discover = DISCOVER_PERIOD,
};

typedef struct{
  bool send_light;
  bool send_motion;
  bool send_periodic;
  bool update_time;
  bool up_time_wait;
  bool resolve_addr;
  bool resolve_wait;
  bool dfu_trigger;
  bool performing_dfu;
  bool do_reset;
  bool has_internet;
  bool external_route_added;
  uint16_t offline_count;
} permamote_state_t;

APP_TIMER_DEF(periodic_sensor_timer);
APP_TIMER_DEF(pir_backoff);
APP_TIMER_DEF(pir_delay);
APP_TIMER_DEF(ntp_jitter);
APP_TIMER_DEF(dfu_monitor);
APP_TIMER_DEF(dns_delay);
APP_TIMER_DEF(coap_tick);

static uint8_t dfu_jitter_hours = 0;
static bool seed = false;
uint8_t device_id[6];
static permamote_state_t state = {0};
static float sensed_lux;

// forward declaration
static void send_thread_info(void);
static void send_version(void);

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);
static nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);

/*
 * methods to manage addresses
 * ==========================
 * */
static bool are_addresses_resolved(void) {
  bool resolved = true;
  for (size_t i = 0; i < NUM_ADDRESSES; i++) {
    resolved &= !otIp6IsAddressEqual(addresses[i], &unspecified_ipv6);
  }
  return resolved;
}

static bool did_resolution_fail(void) {
  bool resolved = false;
  for (size_t i = 0; i < NUM_ADDRESSES; i++) {
    resolved |= addr_fail[i];
  }
  return resolved;
}

static void address_print(const otIp6Address *addr)
{
    char ipstr[40];
    snprintf(ipstr, sizeof(ipstr), "%x:%x:%x:%x:%x:%x:%x:%x",
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 0)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 1)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 2)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 3)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 4)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 5)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 6)),
             uint16_big_decode((uint8_t *)(addr->mFields.m16 + 7)));

    NRF_LOG_INFO("%s\r\n", (uint32_t)ipstr);
}

static void addresses_print(otInstance * aInstance)
{
    for (const otNetifAddress *addr = otIp6GetUnicastAddresses(aInstance); addr; addr = addr->mNext)
    {
        address_print(&addr->mAddress);
    }
}

void coap_dfu_handle_error(void)
{
    coap_dfu_reset_state();
    nrf_dfu_settings_progress_reset();
}

// Function for handling CoAP periodically time ticks.
static void app_coap_time_tick(void* m)
{
    // Pass a tick to coap in order to re-transmit any pending messages.
    (void)coap_time_tick();
}

void dfu_monitor_callback(void* m) {
  background_dfu_diagnostic_t dfu_state;
  coap_dfu_diagnostic_get(&dfu_state);
  NRF_LOG_INFO("state: %s", background_dfu_state_to_string(dfu_state.state));
  NRF_LOG_INFO("prev state: %s", background_dfu_state_to_string(dfu_state.prev_state));

  // if this state combination, there isn't a new image for us, or server not
  // responding, so return to normal operation
  if ((dfu_state.state == BACKGROUND_DFU_DOWNLOAD_TRIG || dfu_state.state == BACKGROUND_DFU_IDLE) &&
      (dfu_state.prev_state == BACKGROUND_DFU_IDLE)) {
    state.performing_dfu = false;
    //app_timer_stop(coap_tick);
    coap_dfu_reset_state();
    nrf_dfu_settings_progress_reset();
    app_timer_stop(dfu_monitor);
    NRF_LOG_INFO("Aborted DFU operation: Image already installed or server not responding");
  }
}

void rtc_callback(void) {
  hours_count++;
  if (hours_count % NTP_UPDATE_HOURS == 0) {
    uint32_t r = rand();
    float jitter = r / (float) RAND_MAX * (NTP_UPDATE_MAX - NTP_UPDATE_MIN) + NTP_UPDATE_MIN;
    NRF_LOG_INFO("jitter: %u", (uint32_t) jitter);
    ret_code_t err_code = app_timer_start(ntp_jitter, APP_TIMER_TICKS(jitter), NULL);
    APP_ERROR_CHECK(err_code);
  }
  if (hours_count % DFU_CHECK_HOURS == dfu_jitter_hours) {
    dfu_jitter_hours = (int)(rand() / (float) RAND_MAX * DFU_CHECK_HOURS);
    NRF_LOG_INFO("jitter: %u", (uint32_t) dfu_jitter_hours);
    NRF_LOG_INFO("going for DFU");
    state.dfu_trigger = true;
  }
}

void ntp_jitter_callback(void* m) {
    state.resolve_addr = true;
    state.update_time = true;
}

void __attribute__((weak)) dns_response_handler(void         * p_context,
                                 const char   * p_hostname,
                                 const otIp6Address * p_resolved_address,
                                 uint32_t       ttl,
                                 otError        error)
{
    uint8_t i = 0;
    for(i=0; i < NUM_ADDRESSES; i++) {
      if (p_context == addresses[i]) {
        break;
      }
    }
    addr_fail[i] = error != OT_ERROR_NONE;

    if (error != OT_ERROR_NONE)
    {
        NRF_LOG_INFO("DNS response error %d.", error);
        return;
    }

    NRF_LOG_INFO("Successfully resolved address");
    memcpy(p_context, p_resolved_address, sizeof(otIp6Address));
}

void ntp_response_handler(void* context, uint64_t time, otError error) {
  struct timeval tv = {.tv_sec = (uint32_t)time & UINT32_MAX};
  if (error == OT_ERROR_NONE) {
    ab1815_set_time(unix_to_ab1815(tv));
    NRF_LOG_INFO("ntp time: %lu.%lu", (uint32_t)time & UINT32_MAX);
  }
  else {
    NRF_LOG_INFO("ntp error: %d", error);
  }
  state.up_time_wait = false;

  // if we haven't performed initial setup of rand
  if (!seed) {
    srand((uint32_t)time & UINT32_MAX);
    // set jitter for next dfu check
    dfu_jitter_hours = (int)(rand() / (float) RAND_MAX * DFU_CHECK_HOURS);
    NRF_LOG_INFO("JITTER: %u", dfu_jitter_hours);
    // send version and discover because why not
    send_thread_info();
    send_version();
    seed = true;
  }
}

void thread_state_changed_callback(uint32_t flags, void * p_context) {
    uint8_t role = otThreadGetDeviceRole(p_context);

    NRF_LOG_INFO("State changed! Flags: 0x%08lx Current role: %d",
        flags, role)

      if (flags & OT_CHANGED_IP6_ADDRESS_ADDED && role == 2) {
        NRF_LOG_INFO("We have internet connectivity!");
        addresses_print(p_context);
        state.external_route_added = true;
        APP_ERROR_CHECK(app_timer_start(dns_delay, APP_TIMER_TICKS(5000), NULL));
        //state.resolve_addr = true;
        //state.update_time = true;
    }
    if (state.external_route_added && role == 2) {
      state.has_internet = true;
      state.offline_count = 0;
    }
    else if (role == 1) {
      state.has_internet = false;
    }
}

//static void send_free_buffers(void) {
//  otBufferInfo buf_info = {0};
//  otMessageGetBufferInfo(thread_get_instance(), &buf_info);
//  printf("Buffer info:\n");
//  printf("\ttotal buffers: %u\n", buf_info.mTotalBuffers);
//  printf("\tfree buffers: %u\n", buf_info.mFreeBuffers);
//  printf("\tIp6 buffers: %u\n", buf_info.mIp6Buffers);
//  printf("\tIp6 messages: %u\n", buf_info.mIp6Messages);
//  printf("\tcoap buffers: %u\n", buf_info.mCoapBuffers);
//  printf("\tcoap messages: %u\n", buf_info.mCoapMessages);
//  printf("\tapp coap buffers: %u\n", buf_info.mApplicationCoapBuffers);
//  printf("\tapp coap messages: %u\n", buf_info.mApplicationCoapMessages);
//  packet.timestamp = ab1815_get_time_unix();
//  packet.data = (uint8_t*)&buf_info.mFreeBuffers;
//  packet.data_len = sizeof(sizeof(uint16_t));
//  gateway_coap_send(&m_coap_address, "free_ot_buffers", false, &packet);
//}

static void send_temp_pres_hum(void) {
  float temperature, pressure, humidity;
  Message msg = Message_init_default;

  // sense temperature and pressure
  nrf_gpio_pin_clear(MS5637_EN);
  ms5637_start();
  ms5637_get_temperature_and_pressure(&temperature, &pressure);
  nrf_gpio_pin_set(MS5637_EN);

  // Prepare temp and pressure packets
  msg.data.temperature_c = temperature;
  msg.data.pressure_mbar = pressure;

  // sense humidity
  nrf_gpio_pin_clear(SI7021_EN);
  nrf_delay_ms(20);
  si7021_config(si7021_mode3);
  si7021_read_RH_hold(&humidity);
  nrf_gpio_pin_set(SI7021_EN);

  // Prepare humidity packet
  msg.data.humidity_percent = humidity;

  gateway_coap_send(&m_coap_address, "temp_pres_hum", false, &msg);

  NRF_LOG_INFO("Sensed ms5637: temperature: %d, pressure: %d", (int32_t)temperature, (int32_t)pressure);
  NRF_LOG_INFO("Sensed si7021: humidity: %d", (int32_t)humidity);
}

static void send_voltage(void) {
  // sense voltage
  Message msg = Message_init_default;

  nrf_saadc_value_t adc_samples[3];
  nrf_drv_saadc_sample_convert(0, adc_samples);
  nrf_drv_saadc_sample_convert(1, adc_samples+1);
  nrf_drv_saadc_sample_convert(2, adc_samples+2);
  msg.data.primary_voltage= 0.6 * 6 * (float)adc_samples[0] / ((1 << 10)-1);
  msg.data.solar_voltage= 0.6 * 6 * (float)adc_samples[1] / ((1 << 10)-1);
  msg.data.secondary_voltage= 0.6 * 6 * (float)adc_samples[2] / ((1 << 10)-1);

  // sense vbat_ok
  msg.data.vbat_ok = nrf_gpio_pin_read(VBAT_OK);

  gateway_coap_send(&m_coap_address, "voltage", false, &msg);

  NRF_LOG_INFO("Sensed voltage: vbat*100: %d, vsol*100: %d, vsec*100: %d", (int32_t)(msg.data.primary_voltage*100), (int32_t)(msg.data.solar_voltage*100), (int32_t)(msg.data.secondary_voltage*100));
  NRF_LOG_INFO("VBAT_OK: %d", msg.data.vbat_ok);
}

void color_read_callback(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear) {
  float cct;
  Message msg = Message_init_default;

  tcs34725_off();
  tcs34725_ir_compensate(&red, &green, &blue, &clear);
  cct = tcs34725_calculate_ct(red, blue);
  msg.data.light_cct_k = cct;
  msg.data.light_counts_red = red;
  msg.data.light_counts_green = green;
  msg.data.light_counts_blue = blue;
  msg.data.light_counts_clear = clear;
  // send
  gateway_coap_send(&m_coap_address, "light_color", false, &msg);

  NRF_LOG_INFO("Sensed light cct: %u", (uint32_t)cct);
  NRF_LOG_INFO("Sensed light color:\n\tr: %u\n\tg: %u\n\tb: %u", (uint16_t)red, (uint16_t)green, (uint16_t)blue);
}
static void send_thread_info(void) {
  otInstance * thread_instance = thread_get_instance();
  Message msg = Message_init_default;

  uint16_t rloc16 = otThreadGetRloc16(thread_instance);
  msg.data.thread_rloc16 = rloc16;
  const otExtAddress * ext_addr = otLinkGetExtendedAddress(thread_instance);
  memcpy(msg.data.thread_ext_addr.bytes, ext_addr, sizeof(otExtAddress));
  msg.data.thread_ext_addr.size = sizeof(otExtAddress);
  int8_t avg_rssi, last_rssi;
  otThreadGetParentAverageRssi(thread_instance, &avg_rssi);
  msg.data.thread_parent_avg_rssi = avg_rssi;
  otThreadGetParentLastRssi(thread_instance, &last_rssi);
  msg.data.thread_parent_last_rssi = last_rssi;

  char ext_str[128];
  snprintf(ext_str, sizeof(ext_str), "%2x:%2x:%2x:%2x:%2x:%2x:%2x:%2x",
          *(ext_addr->m8 + 0),
          *(ext_addr->m8 + 1),
          *(ext_addr->m8 + 2),
          *(ext_addr->m8 + 3),
          *(ext_addr->m8 + 4),
          *(ext_addr->m8 + 5),
          *(ext_addr->m8 + 6),
          *(ext_addr->m8 + 7));

  NRF_LOG_INFO("rloc16: 0x%x", rloc16);
  NRF_LOG_INFO("ext_addr: %s", ext_str);
  NRF_LOG_INFO("average rssi: %d", avg_rssi);
  NRF_LOG_INFO("last rssi: %d", last_rssi);

  gateway_coap_send(&m_coap_address, "thread_info", false, &msg);
}

static void send_version(void) {
  Message msg = Message_init_default;
  strncpy(msg.data.git_version, GIT_VERSION, sizeof(msg.data.git_version));

  NRF_LOG_INFO("Sent version");

  gateway_coap_send(&m_coap_address, "version", false, &msg);
}


static void send_color(void) {
  // sense light color

  tcs34725_on();
  tcs34725_config_agc();
  tcs34725_enable();
  tcs34725_read_channels_agc(color_read_callback);
}

void periodic_sensor_read_callback(void* m) {
  state.send_periodic = true;
}

void light_sensor_read_callback(float lux) {
  sensed_lux = lux;
  state.send_light = true;
}

void pir_backoff_callback(void* m) {
  // turn on and wait for stable
  NRF_LOG_INFO("TURN ON PIR");

  nrf_gpio_pin_clear(PIR_EN);
  ret_code_t err_code = app_timer_start(pir_delay, PIR_DELAY, NULL);
  APP_ERROR_CHECK(err_code);

}

void pir_enable_callback(void* m) {
  // enable interrupt
  NRF_LOG_INFO("TURN ON PIR callback");
  nrf_drv_gpiote_in_event_enable(PIR_OUT, 1);
}

void pir_interrupt_callback(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  state.send_motion = true;
}

void light_interrupt_callback(void) {
    max44009_schedule_read_lux();
}

void send_light() {
  Message msg = Message_init_default;
  float upper = sensed_lux + sensed_lux * 0.1;
  float lower = sensed_lux - sensed_lux * 0.1;
  NRF_LOG_INFO("Sensed: lux: %u, upper: %u, lower: %u", (uint32_t)sensed_lux, (uint32_t)upper, (uint32_t)lower);
  max44009_set_upper_threshold(upper);
  max44009_set_lower_threshold(lower);

  msg.data.light_lux = sensed_lux;
  gateway_coap_send(&m_coap_address, "light_lux", false, &msg);
}

void send_motion() {
  Message msg = Message_init_default;

  // disable interrupt and turn off PIR
  NRF_LOG_INFO("TURN off PIR ");
  nrf_drv_gpiote_in_event_disable(PIR_OUT);
  nrf_gpio_pin_set(PIR_EN);

  ret_code_t err_code = app_timer_start(pir_backoff, PIR_BACKOFF_PERIOD, NULL);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_INFO("Saw motion");
  msg.data.motion = true;

  gateway_coap_send(&m_coap_address, "motion", false, &msg);
}

/**@brief Function for initializing the nrf log module.
 */

//void app_error_fault_handler(uint32_t error_code, __attribute__ ((unused)) uint32_t line_num, __attribute__ ((unused)) uint32_t info) {
//  NRF_LOG_INFO("App error: %d", error_code);
//  uint8_t data[1 + 6 + sizeof(uint32_t)];
//  data[0] = 6;
//  memcpy(data+1, device_id, 6);
//  memcpy(data+1+6, &error_code, sizeof(uint32_t));
//
//  thread_coap_send(thread_get_instance(), OT_COAP_CODE_PUT, OT_COAP_TYPE_NON_CONFIRMABLE, &m_coap_address, "error", data, 1+6+sizeof(uint32_t));
//
//  do_reset = true;
//}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
  // halt all existing state
  __disable_irq();

  // print banner
  printf("\n\n***** App Error *****\n");

  // check cause of error
  switch (id) {
    case NRF_FAULT_ID_SDK_ASSERT: {
        assert_info_t * p_info = (assert_info_t *)info;
        printf("ASSERTION FAILED at %s:%u\n",
            p_info->p_file_name,
            p_info->line_num);
        break;
      }
    case NRF_FAULT_ID_SDK_ERROR: {
        error_info_t * p_info = (error_info_t *)info;
        printf("ERROR %lu [%s] at %s:%lu\t\tPC at: 0x%08lX\n",
            p_info->err_code,
            nrf_strerror_get(p_info->err_code),
            p_info->p_file_name,
            p_info->line_num,
            pc);
        break;
      }
    default: {
      printf("UNKNOWN FAULT at 0x%08lX\n", pc);
      break;
    }
  }


  //uint8_t data[1 + 6 + sizeof(uint32_t)];
  //data[0] = 6;
  //memcpy(data+1, device_id, 6);
  //memcpy(data+1+6, &error_code, sizeof(uint32_t));

  //thread_coap_send(thread_get_instance(), OT_COAP_CODE_PUT, OT_COAP_TYPE_NON_CONFIRMABLE, &m_coap_address, "error", data, 1+6+sizeof(uint32_t));
  NRF_LOG_INFO("GOING FOR RESET");
  NRF_LOG_FINAL_FLUSH();
  state.do_reset = true;
}

void state_step(void) {
  otInstance * thread_instance = thread_get_instance();
  bool ready_to_send = state.has_internet && are_addresses_resolved();
  uint32_t poll = otLinkGetPollPeriod(thread_get_instance());

  if (state.send_periodic) {
    ab1815_tickle_watchdog();
    period_count ++;
    NRF_LOG_INFO("poll period: %d", poll);

    if (!ready_to_send) {
      // keep a count of how long device has been offline
      state.offline_count++;
      // attempt a reset if it's been too long
      if (state.offline_count > MAX_OFFLINE_MINS) {
        state.do_reset = true;
      }
    }
    else {
      if (period_count % sensor_period.voltage == 0) {
        send_voltage();
      }
      if (period_count % sensor_period.temp_pres_hum == 0) {
        send_temp_pres_hum();
      }
      if (period_count % sensor_period.color == 0) {
        send_color();
        max44009_schedule_read_lux();
      }
      if (period_count % sensor_period.discover == 0) {
        send_thread_info();
      }

      if (state.dfu_trigger == true && state.performing_dfu == false) {
        state.dfu_trigger = false;
        state.performing_dfu = true;

        // Send discovery packet and version before updating
        send_version();

        coap_remote_t remote;
        memcpy(&remote.addr, &m_up_address, OT_IP6_ADDRESS_SIZE);
        remote.port_number = 5683;
        ret_code_t err_code = app_timer_start(dfu_monitor, DFU_MONITOR_PERIOD, NULL);
        APP_ERROR_CHECK(err_code);
        int result = coap_dfu_trigger(&remote);
        NRF_LOG_INFO("result: %d", result);
        if (result == NRF_ERROR_INVALID_STATE) {
          coap_dfu_reset_state();
          nrf_dfu_settings_progress_reset();
        }
      }
    }

    state.send_periodic = false;
  }

  if (state.has_internet) {
    if (state.resolve_addr) {
      NRF_LOG_INFO("Resolving Addresses");
      thread_dns_hostname_resolve(thread_instance,
          DNS_SERVER_ADDR,
          NTP_SERVER_HOSTNAME,
          dns_response_handler,
          (void*) &m_ntp_address);
      thread_dns_hostname_resolve(thread_instance,
          DNS_SERVER_ADDR,
          COAP_SERVER_HOSTNAME,
          dns_response_handler,
          (void*) &m_coap_address);
      thread_dns_hostname_resolve(thread_instance,
          DNS_SERVER_ADDR,
          UPDATE_SERVER_HOSTNAME,
          dns_response_handler,
          (void*) &m_up_address);
      state.resolve_addr = false;
      state.resolve_wait = true;
    }
    if (state.resolve_wait) {
      if (are_addresses_resolved() || did_resolution_fail())
      {
        state.resolve_wait = false;
      }
    }
  }

  if (ready_to_send) {
    if (state.send_light) {
      state.send_light = false;
      send_light();
    }
    if (state.send_motion) {
      state.send_motion = false;
      send_motion();
    }

    if (state.update_time && !state.resolve_wait && !state.resolve_addr && are_addresses_resolved()) {
      state.update_time = false;
      state.up_time_wait = true;
      NRF_LOG_INFO("RTC UPDATE");

      // request ntp time update
      otError error = thread_ntp_request(thread_instance, &m_ntp_address, NULL, ntp_response_handler);
      NRF_LOG_INFO("sent ntp poll!");
      NRF_LOG_INFO("error: %d", error);
    }
  }

  // manage poll period. if we are waiting for any transfer (dns, ntp, etc.) we want a fast poll rate, otherwise, a slow one.
  if(state.resolve_wait || state.up_time_wait || state.performing_dfu) {
    if (poll != RECV_POLL_PERIOD) {
      otLinkSetPollPeriod(thread_instance, RECV_POLL_PERIOD);
    }

  } else if (poll != DEFAULT_POLL_PERIOD){
    otLinkSetPollPeriod(thread_instance, DEFAULT_POLL_PERIOD);
  }

  if (state.do_reset) {
    static volatile int i = 0;
    if (i++ > 100) {
      NVIC_SystemReset();
    }
  }
}

void timer_init(void)
{
  ret_code_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&periodic_sensor_timer, APP_TIMER_MODE_REPEATED, periodic_sensor_read_callback);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_start(periodic_sensor_timer, SENSOR_PERIOD, NULL);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&pir_backoff, APP_TIMER_MODE_SINGLE_SHOT, pir_backoff_callback);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&pir_delay, APP_TIMER_MODE_SINGLE_SHOT, pir_enable_callback);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&ntp_jitter, APP_TIMER_MODE_SINGLE_SHOT, ntp_jitter_callback);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&dfu_monitor, APP_TIMER_MODE_REPEATED, dfu_monitor_callback);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&dns_delay, APP_TIMER_MODE_SINGLE_SHOT, ntp_jitter_callback);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&coap_tick, APP_TIMER_MODE_REPEATED, app_coap_time_tick);
  APP_ERROR_CHECK(err_code);
  // Start coap timer tick
  err_code = app_timer_start(coap_tick, COAP_TICK_TIME, NULL);
  APP_ERROR_CHECK(err_code);


}

int main(void) {
  // init softdevice
  //nrf_sdh_enable_request();
  //sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  nrf_power_dcdcen_set(1);

  // Init log
  log_init();
  NRF_LOG_INFO("\n");
  flash_storage_init();
  nrf_mem_init();

  // Init twi
  twi_init(&twi_mngr_instance);
  adc_init();

  // init periodic timers
  timer_init();

  NRF_LOG_INFO("GIT Version: %s", GIT_VERSION);
  uint8_t id[6] = {0};
  get_device_id(id);
  // store ID with fds
  define_flash_variable_array(id, device_id, sizeof(id), ID_LOCATOR);
  NRF_LOG_INFO("Device ID: %x:%x:%x:%x:%x:%x", device_id[0], device_id[1],
                device_id[2], device_id[3], device_id[4], device_id[5]);

  // setup thread
  thread_config_t thread_config = {
    .channel = 25,
    .panid = 0xFACE,
    .sed = true,
    .poll_period = DEFAULT_POLL_PERIOD,
    .child_period = DEFAULT_CHILD_TIMEOUT,
    .autocommission = true,
  };

  thread_init(&thread_config);
  otInstance* thread_instance = thread_get_instance();
  coap_dfu_init(thread_instance);
  thread_coap_client_init(thread_instance);
  for(uint8_t i = 0; i < NUM_ADDRESSES; i++) {
    *(addresses[i]) = unspecified_ipv6;
  }

  sensors_init(&twi_mngr_instance, &spi_instance);

  // enable interrupts for pir, light sensor
  ret_code_t err_code = app_timer_start(pir_delay, PIR_DELAY, NULL);
  APP_ERROR_CHECK(err_code);
  max44009_schedule_read_lux();
  max44009_enable_interrupt();

  state.dfu_trigger = true;
  while (1) {
    coap_dfu_process();
    thread_process();
    state_step();
    if (NRF_LOG_PROCESS() == false)
    {
      thread_sleep();
    }
  }
}

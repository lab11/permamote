#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
#include "nrf_dfu_utils.h"
#include "coap_dfu.h"
#include "background_dfu_state.h"

#include <openthread/message.h>

#include "simple_thread.h"
#include "permamote_coap.h"
#include "thread_coap.h"
#include "thread_dns.h"
#include "device_id.h"
#include "ntp.h"

#include "permamote.h"
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
static otIp6Address m_ntp_address =
{
    .mFields =
    {
        .m8 = {0}
    }
};
static otIp6Address m_peer_address =
{
    .mFields =
    {
        .m8 = {0}
    }
};

/*
 * methods to print addresses
 * ==========================
 * */
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

/*
 * Permamote specific declarations for packet structure, state machine
 * ==========================
 * */
static permamote_packet_t packet = {
    .id = NULL,
    .id_len = 0,
    .data = NULL,
    .data_len = 0,
};

typedef enum {
  IDLE = 0,
  SEND_LIGHT,
  SEND_MOTION,
  SEND_PERIODIC,
  SEND_DISCOVERY,
  UPDATE_TIME,
} permamote_state_t;

APP_TIMER_DEF(discover_send_timer);
APP_TIMER_DEF(periodic_sensor_timer);
APP_TIMER_DEF(pir_backoff);
APP_TIMER_DEF(pir_delay);
APP_TIMER_DEF(rtc_update_first);

static bool trigger = true;
static uint8_t device_id[6];
static otNetifAddress m_slaac_addresses[6]; /**< Buffer containing addresses resolved by SLAAC */
static struct ntp_client_t ntp_client;
static permamote_state_t state = IDLE;
static float sensed_lux;
static bool do_reset = false;

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);
static nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);

void coap_dfu_handle_error(void)
{
    coap_dfu_reset_state();
}

void ntp_recv_callback(struct ntp_client_t* client) {
  if (client->state == NTP_CLIENT_RECV) {
    ab1815_set_time(unix_to_ab1815(client->tv));
    NRF_LOG_INFO("ntp time: %lu.%lu", client->tv.tv_sec, client->tv.tv_usec);
  }
  else {
    NRF_LOG_INFO("ntp error state: 0x%x", client->state);
  }
  otLinkSetPollPeriod(thread_get_instance(), DEFAULT_POLL_PERIOD);
}

static void dns_response_handler(void         * p_context,
                                 const char   * p_hostname,
                                 otIp6Address * p_resolved_address,
                                 uint32_t       ttl,
                                 otError        error)
{
    if (error != OT_ERROR_NONE)
    {
        NRF_LOG_INFO("DNS response error %d.", error);
        return;
    }

    NRF_LOG_INFO("Successfully resolved address");
    memcpy(p_context, p_resolved_address, sizeof(otIp6Address));
}

void rtc_update_callback() {
  state = UPDATE_TIME;
}

void __attribute__((weak)) thread_state_changed_callback(uint32_t flags, void * p_context) {
    NRF_LOG_INFO("State changed! Flags: 0x%08lx Current role: %d",
                 flags, otThreadGetDeviceRole(p_context));

    if (flags & OT_CHANGED_THREAD_NETDATA)
    {
        /**
         * Whenever Thread Network Data is changed, it is necessary to check if generation of global
         * addresses is needed. This operation is performed internally by the OpenThread CLI module.
         * To lower power consumption, the examples that implement Thread Sleepy End Device role
         * don't use the OpenThread CLI module. Therefore otIp6SlaacUpdate util is used to create
         * IPv6 addresses.
         */
         otIp6SlaacUpdate(p_context, m_slaac_addresses,
                          sizeof(m_slaac_addresses) / sizeof(m_slaac_addresses[0]),
                          otIp6CreateRandomIid, NULL);
    }
    if (flags & OT_CHANGED_IP6_ADDRESS_ADDED && otThreadGetDeviceRole(p_context) == 2) {
      NRF_LOG_INFO("We have internet connectivity!");
      otLinkSetPollPeriod(p_context, RECV_POLL_PERIOD);

      // resolve dns
      thread_dns_hostname_resolve(p_context,
                                  DNS_SERVER_ADDR,
                                  NTP_SERVER_HOSTNAME,
                                  dns_response_handler,
                                  (void*) &m_ntp_address);
      thread_dns_hostname_resolve(p_context,
                                  DNS_SERVER_ADDR,
                                  COAP_SERVER_HOSTNAME,
                                  dns_response_handler,
                                  (void*) &m_peer_address);

      int err_code = app_timer_start(rtc_update_first, RTC_UPDATE_FIRST, NULL);
      APP_ERROR_CHECK(err_code);

      addresses_print(p_context);
    }
}

void discover_send_callback(void* m) {
  state = SEND_DISCOVERY;
}

static void send_free_buffers(void) {
  otBufferInfo buf_info = {0};
  otMessageGetBufferInfo(thread_get_instance(), &buf_info);
  printf("Buffer info:\n");
  printf("\ttotal buffers: %u\n", buf_info.mTotalBuffers);
  printf("\tfree buffers: %u\n", buf_info.mFreeBuffers);
  printf("\tIp6 buffers: %u\n", buf_info.mIp6Buffers);
  printf("\tIp6 messages: %u\n", buf_info.mIp6Messages);
  printf("\tcoap buffers: %u\n", buf_info.mCoapBuffers);
  printf("\tcoap messages: %u\n", buf_info.mCoapMessages);
  printf("\tapp coap buffers: %u\n", buf_info.mApplicationCoapBuffers);
  printf("\tapp coap messages: %u\n", buf_info.mApplicationCoapMessages);
  packet.timestamp = ab1815_get_time_unix();
  packet.data = (uint8_t*)&buf_info.mFreeBuffers;
  packet.data_len = sizeof(sizeof(uint16_t));
  permamote_coap_send(&m_peer_address, "free_ot_buffers", false, &packet);
}

static void send_temp_pres_hum(void) {
  float temperature, pressure, humidity;
  packet.data_len = sizeof(temperature);

  // sense temperature and pressure
  nrf_gpio_pin_clear(MS5637_EN);
  ms5637_start();
  ms5637_get_temperature_and_pressure(&temperature, &pressure);
  nrf_gpio_pin_set(MS5637_EN);
  // Prepare temp and pressure packets
  packet.timestamp = ab1815_get_time_unix();
  packet.data = (uint8_t*)&temperature;
  // send and tickle if success
  permamote_coap_send(&m_peer_address, "temperature_c", false, &packet);
  packet.data = (uint8_t*)&pressure;
  permamote_coap_send(&m_peer_address, "pressure_mbar", false, &packet);
  NRF_LOG_INFO("Sensed ms5637: temperature: %d, pressure: %d", (int32_t)temperature, (int32_t)pressure);

  // sense humidity
  nrf_gpio_pin_clear(SI7021_EN);
  nrf_delay_ms(20);
  si7021_config(si7021_mode3);
  si7021_read_RH_hold(&humidity);
  nrf_gpio_pin_set(SI7021_EN);
  // Prepare humidity packet
  packet.timestamp = ab1815_get_time_unix();
  packet.data = (uint8_t*)&humidity;
  // send and tickle if success
  permamote_coap_send(&m_peer_address, "humidity_percent", false, &packet);
  NRF_LOG_INFO("Sensed si7021: humidity: %d", (int32_t)humidity);
}

static void send_voltage(void) {
  // sense voltage
  nrf_saadc_value_t adc_samples[3];
  nrf_drv_saadc_sample_convert(0, adc_samples);
  nrf_drv_saadc_sample_convert(1, adc_samples+1);
  nrf_drv_saadc_sample_convert(2, adc_samples+2);
  float vbat = 0.6 * 6 * (float)adc_samples[0] / ((1 << 10)-1);
  float vsol = 0.6 * 6 * (float)adc_samples[1] / ((1 << 10)-1);
  float vsec = 0.6 * 6 * (float)adc_samples[2] / ((1 << 10)-1);
  // prepare voltage packet
  packet.timestamp = ab1815_get_time_unix();
  float v_data[3] = {vbat, vsol, vsec};
  packet.data = (uint8_t*)v_data;
  packet.data_len = 3 * sizeof(vbat);
  // send and tickle if success
  permamote_coap_send(&m_peer_address, "voltage", false, &packet);
  NRF_LOG_INFO("Sensed voltage: vbat*100: %d, vsol*100: %d, vsec*100: %d", (int32_t)(vbat*100), (int32_t)(vsol*100), (int32_t)(vsec*100));

  // sense vbat_ok
  bool vbat_ok = nrf_gpio_pin_read(VBAT_OK);
  packet.data = (uint8_t*)&vbat_ok;
  packet.data_len = sizeof(vbat_ok);
  permamote_coap_send(&m_peer_address, "vbat_ok", false, &packet);
  NRF_LOG_INFO("VBAT_OK: %d", vbat_ok);
}

void color_read_callback(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear) {
  float cct;

  tcs34725_off();
  tcs34725_ir_compensate(&red, &green, &blue, &clear);
  //cct = tcs34725_calculate_cct(red, green, blue);
  cct = tcs34725_calculate_ct(red, blue);
  packet.timestamp = ab1815_get_time_unix();
  packet.data = (uint8_t*)&cct;
  packet.data_len = sizeof(cct);
  // send
  permamote_coap_send(&m_peer_address, "light_color_cct_k", false, &packet);

  uint16_t lraw_data[4] = {red, green, blue, clear};
  packet.data = (uint8_t*)lraw_data;
  packet.data_len = 4*sizeof(red);
  permamote_coap_send(&m_peer_address, "light_color_counts", false, &packet);

  NRF_LOG_INFO("Sensed light cct: %u", (uint32_t)cct);
  NRF_LOG_INFO("Sensed light color:\n\tr: %u\n\tg: %u\n\tb: %u", (uint16_t)red, (uint16_t)green, (uint16_t)blue);
}

static void send_color(void) {
  // sense light color

  tcs34725_on();
  tcs34725_config_agc();
  tcs34725_enable();
  tcs34725_read_channels_agc(color_read_callback);
}

void periodic_sensor_read_callback(void* m) {
  ab1815_time_t time;
  time = unix_to_ab1815(packet.timestamp);
  //NRF_LOG_INFO("time: %d:%02d:%02d, %d/%d/20%02d", time.hours, time.minutes, time.seconds, time.months, time.date, time.years);
  if(time.years == 0) {
    NRF_LOG_INFO("VERY INVALID TIME");
    int err_code = app_timer_start(rtc_update_first, RTC_UPDATE_FIRST, NULL);
    APP_ERROR_CHECK(err_code);
  }
  if (otThreadGetDeviceRole(thread_get_instance()) == 2) {
    ab1815_tickle_watchdog();
  }

  state = SEND_PERIODIC;
}

void light_sensor_read_callback(float lux) {
  sensed_lux = lux;
  state = SEND_LIGHT;
}

void pir_backoff_callback(void* m) {
  // turn on and wait for stable
  NRF_LOG_INFO("TURN ON PIR");

  nrf_gpio_pin_clear(PIR_EN);
  uint32_t err_code = app_timer_start(pir_delay, PIR_DELAY, NULL);
  APP_ERROR_CHECK(err_code);

}

void pir_enable_callback(void* m) {
  // enable interrupt
  NRF_LOG_INFO("TURN ON PIR callback");
  nrf_drv_gpiote_in_event_enable(PIR_OUT, 1);
}

void pir_interrupt_callback(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  // disable interrupt and turn off PIR
  NRF_LOG_INFO("TURN off PIR ");
  nrf_drv_gpiote_in_event_disable(PIR_OUT);
  nrf_drv_gpiote_in_event_enable(PIR_OUT, 0);
  nrf_gpio_pin_set(PIR_EN);

  uint32_t err_code = app_timer_start(pir_backoff, PIR_BACKOFF_PERIOD, NULL);
  APP_ERROR_CHECK(err_code);

  state = SEND_MOTION;
}

void light_interrupt_callback(void) {
    max44009_schedule_read_lux();
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
//  thread_coap_send(thread_get_instance(), OT_COAP_CODE_PUT, OT_COAP_TYPE_NON_CONFIRMABLE, &m_peer_address, "error", data, 1+6+sizeof(uint32_t));
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

  //thread_coap_send(thread_get_instance(), OT_COAP_CODE_PUT, OT_COAP_TYPE_NON_CONFIRMABLE, &m_peer_address, "error", data, 1+6+sizeof(uint32_t));
  NRF_LOG_INFO("GOING FOR RESET");
  NRF_LOG_FINAL_FLUSH();
  do_reset = true;
}

void state_step(void) {
  switch(state) {
    case SEND_LIGHT:{
      float upper = sensed_lux + sensed_lux * 0.05;
      float lower = sensed_lux - sensed_lux * 0.05;
      NRF_LOG_INFO("Sensed: lux: %u, upper: %u, lower: %u", (uint32_t)sensed_lux, (uint32_t)upper, (uint32_t)lower);
      max44009_set_upper_threshold(upper);
      max44009_set_lower_threshold(lower);

      packet.timestamp = ab1815_get_time_unix();
      packet.data = (uint8_t*)&sensed_lux;
      packet.data_len = sizeof(sensed_lux);
      permamote_coap_send(&m_peer_address, "light_lux", false, &packet);
      state = IDLE;
      break;
    }
    case SEND_MOTION: {
      uint8_t data = 1;
      NRF_LOG_INFO("Saw motion");
      packet.timestamp = ab1815_get_time_unix();
      packet.data = &data;
      packet.data_len = sizeof(data);
      permamote_coap_send(&m_peer_address, "motion", false, &packet);

      state = IDLE;
      break;
    }
    case SEND_PERIODIC: {
      //NRF_LOG_INFO("poll period: %d", otLinkGetPollPeriod(thread_get_instance()));
      //send_free_buffers();
      //send_temp_pres_hum();
      //send_voltage();
      //send_color();

      state = IDLE;

      //max44009_schedule_read_lux();
      if (trigger == true) {
        //trigger = false;
        background_dfu_diagnostic_t dfu_state;
        coap_dfu_diagnostic_get(&dfu_state);
        NRF_LOG_INFO("state: %d", dfu_state.state);
        NRF_LOG_INFO("prev state: %d", dfu_state.prev_state);
        NRF_LOG_INFO("trigger: %d", dfu_state.triggers_received);
        otLinkSetPollPeriod(thread_get_instance(), RECV_POLL_PERIOD);
        int result = coap_dfu_trigger(NULL);
        NRF_LOG_INFO("result: %d", result);
        //if (result == NRF_ERROR_INVALID_STATE) {
        //    coap_dfu_reset_state();
        //}
      }
      break;
    }
    case UPDATE_TIME: {
      NRF_LOG_INFO("RTC UPDATE");
      NRF_LOG_INFO("sent ntp poll!");
      int error = ntp_client_begin(thread_get_instance(), &ntp_client, &m_ntp_address, 123, 127, ntp_recv_callback, NULL);
      NRF_LOG_INFO("error: %d", error);
      if (error) {
        memset(&ntp_client, 0, sizeof(struct ntp_client_t));
        otLinkSetPollPeriod(thread_get_instance(), DEFAULT_POLL_PERIOD);
        return;
      }
      otLinkSetPollPeriod(thread_get_instance(), RECV_POLL_PERIOD);

      state = IDLE;
      break;
    }
    case SEND_DISCOVERY: {
      const char* addr = PARSE_ADDR;
      uint8_t addr_len = strlen(addr);
      uint8_t data[addr_len + 1];

      NRF_LOG_INFO("Sent discovery");

      data[0] = addr_len;
      memcpy(data+1, addr, addr_len);
      packet.timestamp = ab1815_get_time_unix();
      packet.data = data;
      packet.data_len = addr_len + 1;

      permamote_coap_send(&m_peer_address, "discovery", false, &packet);

      state = IDLE;
      break;
    }
    default:
      break;
  }

  if (do_reset) {
    static volatile int i = 0;
    if (i++ > 100) {
      NVIC_SystemReset();
    }
  }
}

void timer_init(void)
{
  uint32_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&discover_send_timer, APP_TIMER_MODE_REPEATED, discover_send_callback);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_start(discover_send_timer, DISCOVER_PERIOD, NULL);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&periodic_sensor_timer, APP_TIMER_MODE_REPEATED, periodic_sensor_read_callback);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_start(periodic_sensor_timer, SENSOR_PERIOD, NULL);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&rtc_update_first, APP_TIMER_MODE_SINGLE_SHOT, rtc_update_callback);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&pir_backoff, APP_TIMER_MODE_SINGLE_SHOT, pir_backoff_callback);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&pir_delay, APP_TIMER_MODE_SINGLE_SHOT, pir_enable_callback);
  APP_ERROR_CHECK(err_code);
}



int main(void) {
  // init softdevice
  //nrf_sdh_enable_request();
  //sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  nrf_power_dcdcen_set(1);

  // Init log
  log_init();

  nrf_mem_init();

  // Init twi
  twi_init(&twi_mngr_instance);
  adc_init();

  // init periodic timers
  timer_init();

  get_device_id(device_id);
  packet.id = device_id;
  packet.id_len = sizeof(device_id);

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
  //thread_coap_client_init(thread_instance);

  sensors_init(&twi_mngr_instance, &spi_instance);
  uint32_t err_code = app_timer_start(pir_delay, PIR_DELAY, NULL);
  APP_ERROR_CHECK(err_code);

  while (1) {
    coap_dfu_process();
    thread_process();
    ntp_client_process(&ntp_client);
    state_step();
    if (NRF_LOG_PROCESS() == false)
    {
      thread_sleep();
    }
  }
}

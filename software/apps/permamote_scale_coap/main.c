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
#include "nrf_log_default_backends.h"
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
extern otIp6Address unspecified_ipv6;
static otIp6Address m_ntp_address;
static otIp6Address m_coap_address;
#define NUM_ADDRESSES 2
static otIp6Address* addresses[NUM_ADDRESSES] = {&m_ntp_address, &m_coap_address};

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
typedef struct {
  uint8_t voltage;
  uint8_t temp_pres_hum;
  uint8_t weight;
  uint8_t color;
  uint8_t discover;
} permamote_sensor_period_t;
uint32_t period_count = 0;

static permamote_sensor_period_t sensor_period = {
  .voltage = VOLTAGE_PERIOD,
  .temp_pres_hum = TPH_PERIOD,
  .weight = WEIGHT_PERIOD,
  .color = COLOR_PERIOD,
  .discover = DISCOVER_PERIOD,
};

static permamote_packet_t packet = {
    .id = NULL,
    .id_len = 0,
    .data = NULL,
    .data_len = 0,
};

typedef enum {
  IDLE = 0,
  SEND_LIGHT,
  SEND_PERIODIC,
  UPDATE_TIME,
  RESOLVING_ADDR,
  WAIT_TIME,
} permamote_state_t;

APP_TIMER_DEF(periodic_sensor_timer);

static bool trigger = false;
static uint8_t device_id[6];
static otNetifAddress m_slaac_addresses[6]; /**< Buffer containing addresses resolved by SLAAC */
static struct ntp_client_t ntp_client;
static permamote_state_t state = IDLE;
static float sensed_lux;
static bool do_reset = false;
static int dns_error = 0;

#define HX711_DATA UART_TXD
#define HX711_CLK UART_RXD

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);
static nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);

void coap_dfu_handle_error(void)
{
    coap_dfu_reset_state();
}

void rtc_update_callback(void) {
    if (state == IDLE) {
        state = UPDATE_TIME;
    }
}

void __attribute__((weak)) dns_response_handler(void         * p_context,
                                 const char   * p_hostname,
                                 otIp6Address * p_resolved_address,
                                 uint32_t       ttl,
                                 otError        error)
{
    dns_error |= error;
    if (error != OT_ERROR_NONE)
    {
        NRF_LOG_INFO("DNS response error %d.", error);
        return;
    }

    NRF_LOG_INFO("Successfully resolved address");
    memcpy(p_context, p_resolved_address, sizeof(otIp6Address));
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
  state = IDLE;
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
      addresses_print(p_context);
      if (state == IDLE) {
        state = UPDATE_TIME;
      }
    }
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
  permamote_coap_send(&m_coap_address, "free_ot_buffers", false, &packet);
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
  permamote_coap_send(&m_coap_address, "temperature_c", false, &packet);
  packet.data = (uint8_t*)&pressure;
  permamote_coap_send(&m_coap_address, "pressure_mbar", false, &packet);
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
  permamote_coap_send(&m_coap_address, "humidity_percent", false, &packet);
  NRF_LOG_INFO("Sensed si7021: humidity: %d", (int32_t)humidity);
}

static void send_weight(void) {
  int32_t weight = 0xFF000000;
  int32_t weight_sum = 0;
  packet.data_len = sizeof(weight);

  // weight

  //turn on the scale and pull the CLK low to start conversion
  nrf_gpio_pin_clear(PIR_EN);
  nrf_gpio_pin_clear(HX711_CLK);

  //wait 400ms for the output to settle
  nrf_delay_ms(10);

  while(nrf_gpio_pin_read(HX711_DATA));
  nrf_delay_ms(1);

  for(uint8_t j = 0; j < 20; j++) {
    //start reading the weight
    for(uint8_t i = 0; i < 25; i++) {
      nrf_gpio_pin_set(HX711_CLK);
      nrf_delay_us(1);
      nrf_gpio_pin_clear(HX711_CLK);
      if(i <= 23) {
        weight |= (nrf_gpio_pin_read(HX711_DATA) << (23-i));
      }
      nrf_delay_us(1);
    }

    weight_sum += weight;
    weight = 0xFF000000;

    //wait 400ms for the output to settle
    while(nrf_gpio_pin_read(HX711_DATA));
    nrf_delay_ms(1);
  }

  //disable the weight sensor
  nrf_gpio_pin_set(PIR_EN);

  float final_weight = weight_sum/20.0;
  NRF_LOG_INFO("final raw weight: %d", (int32_t)final_weight);

  //this was done empirically
  float weight_g = (final_weight - -16624343)/755.32;
  int32_t weight_mg = (int32_t)(weight_g*1000);

  // Prepare temp and pressure packets
  packet.timestamp = ab1815_get_time_unix();
  packet.data = (uint8_t*)&weight_mg;
  // send and tickle if success
  permamote_coap_send(&m_coap_address, "weight_mg", false, &packet);
  NRF_LOG_INFO("Sensed weight mg: %d", weight_mg);
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
  permamote_coap_send(&m_coap_address, "voltage", false, &packet);
  NRF_LOG_INFO("Sensed voltage: vbat*100: %d, vsol*100: %d, vsec*100: %d", (int32_t)(vbat*100), (int32_t)(vsol*100), (int32_t)(vsec*100));

  // sense vbat_ok
  bool vbat_ok = nrf_gpio_pin_read(VBAT_OK);
  packet.data = (uint8_t*)&vbat_ok;
  packet.data_len = sizeof(vbat_ok);
  permamote_coap_send(&m_coap_address, "vbat_ok", false, &packet);
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
  permamote_coap_send(&m_coap_address, "light_color_cct_k", false, &packet);

  uint16_t lraw_data[4] = {red, green, blue, clear};
  packet.data = (uint8_t*)lraw_data;
  packet.data_len = 4*sizeof(red);
  permamote_coap_send(&m_coap_address, "light_color_counts", false, &packet);

  NRF_LOG_INFO("Sensed light cct: %u", (uint32_t)cct);
  NRF_LOG_INFO("Sensed light color:\n\tr: %u\n\tg: %u\n\tb: %u", (uint16_t)red, (uint16_t)green, (uint16_t)blue);
}

static void send_discover(void) {
  const char* addr = PARSE_ADDR;
  uint8_t addr_len = strlen(addr);
  uint8_t data[addr_len + 1];

  NRF_LOG_INFO("Sent discovery");

  data[0] = addr_len;
  memcpy(data+1, addr, addr_len);
  packet.timestamp = ab1815_get_time_unix();
  packet.data = data;
  packet.data_len = addr_len + 1;

  permamote_coap_send(&m_coap_address, "discovery", false, &packet);
}

static void send_color(void) {
  // sense light color

  tcs34725_on();
  tcs34725_config_agc();
  tcs34725_enable();
  tcs34725_read_channels_agc(color_read_callback);
}

void periodic_sensor_read_callback(void* m) {
  if (state == IDLE) {
    state = SEND_PERIODIC;
  }
}

void light_sensor_read_callback(float lux) {
  sensed_lux = lux;
  if (state == IDLE) {
    state = SEND_LIGHT;
  }
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
  do_reset = true;
}

void state_step(void) {
  otInstance * thread_instance = thread_get_instance();
  switch(state) {
    case IDLE: {
      //uint32_t poll = otLinkGetPollPeriod(thread_get_instance());
      //if (poll != DEFAULT_POLL_PERIOD) {
      //  otLinkSetPollPeriod(thread_instance, DEFAULT_POLL_PERIOD);
      //}
    break;
    }
    case SEND_LIGHT:{
      float upper = sensed_lux + sensed_lux * 0.05;
      float lower = sensed_lux - sensed_lux * 0.05;
      NRF_LOG_INFO("Sensed: lux: %u, upper: %u, lower: %u", (uint32_t)sensed_lux, (uint32_t)upper, (uint32_t)lower);
      max44009_set_upper_threshold(upper);
      max44009_set_lower_threshold(lower);

      packet.timestamp = ab1815_get_time_unix();
      packet.data = (uint8_t*)&sensed_lux;
      packet.data_len = sizeof(sensed_lux);
      permamote_coap_send(&m_coap_address, "light_lux", false, &packet);
      state = IDLE;

      break;
    }
    case SEND_PERIODIC: {
      //NRF_LOG_INFO("poll period: %d", otLinkGetPollPeriod(thread_get_instance()));
      //ab1815_time_t time;
      //ab1815_get_time(&time);
      //NRF_LOG_INFO("time: %d:%02d:%02d, %d/%d/20%02d", time.hours, time.minutes, time.seconds, time.months, time.date, time.years);
      //if (otThreadGetDeviceRole(thread_get_instance()) == 2) {
      //  ab1815_tickle_watchdog();
      //}
      //if(time.years == 0 && state == IDLE) {
      //  NRF_LOG_INFO("VERY INVALID TIME");
      //  state = UPDATE_TIME;
      //  return;
      //}


      period_count ++;
      if (period_count % sensor_period.voltage == 0) {
        send_voltage();
      }
      if (period_count % sensor_period.temp_pres_hum == 0) {
        send_temp_pres_hum();
      }
      if (period_count % sensor_period.weight == 0) {
        send_weight();
      }
      if (period_count % sensor_period.color == 0) {
        send_color();
      }
      if (period_count % sensor_period.discover == 0) {
        send_discover();
      }
      //send_free_buffers();

      state = IDLE;

      //max44009_schedule_read_lux();
      if (trigger == true) {
        trigger = false;
        background_dfu_diagnostic_t dfu_state;
        coap_dfu_diagnostic_get(&dfu_state);
        NRF_LOG_INFO("state: %d", dfu_state.state);
        NRF_LOG_INFO("prev state: %d", dfu_state.prev_state);
        NRF_LOG_INFO("trigger: %d", dfu_state.triggers_received);

        otLinkSetPollPeriod(thread_instance, RECV_POLL_PERIOD);
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
      otLinkSetPollPeriod(thread_instance, RECV_POLL_PERIOD);

      // resolve dns if needed
      if (otIp6IsAddressEqual(&m_ntp_address, &unspecified_ipv6)
       || otIp6IsAddressEqual(&m_coap_address, &unspecified_ipv6)) {
        NRF_LOG_INFO("Resolving Addresses");
        dns_error = 0;
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
        state = RESOLVING_ADDR;
        break;
      }

      int error = ntp_client_begin(thread_instance, &ntp_client, &m_ntp_address, 123, 127, ntp_recv_callback, NULL);
      NRF_LOG_INFO("sent ntp poll!");
      NRF_LOG_INFO("error: %d", error);
      if (error) {
        memset(&ntp_client, 0, sizeof(struct ntp_client_t));
        state = IDLE;
        return;
      }

      state = WAIT_TIME;
      break;
    }
    case RESOLVING_ADDR: {
      if (!(otIp6IsAddressEqual(&m_ntp_address, &unspecified_ipv6)
       || otIp6IsAddressEqual(&m_coap_address, &unspecified_ipv6)) ||
       dns_error != 0)
      {
         state = UPDATE_TIME;
      }
      break;
    }
    case WAIT_TIME: {
      uint32_t poll = otLinkGetPollPeriod(thread_get_instance());
      if (poll != RECV_POLL_PERIOD) {
        NRF_LOG_INFO("poll: %u", poll);
        otLinkSetPollPeriod(thread_instance, RECV_POLL_PERIOD);
      }
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

  err_code = app_timer_create(&periodic_sensor_timer, APP_TIMER_MODE_REPEATED, periodic_sensor_read_callback);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_start(periodic_sensor_timer, SENSOR_PERIOD, NULL);
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
  NRF_LOG_INFO("Device ID: %x:%x:%x:%x:%x:%x", device_id[0], device_id[1],
                device_id[2], device_id[3], device_id[4], device_id[5]);
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
  for(uint8_t i = 0; i < NUM_ADDRESSES; i++) {
    *(addresses[i]) = unspecified_ipv6;
  }

  sensors_init(&twi_mngr_instance, &spi_instance);

  max44009_schedule_read_lux();
  max44009_enable_interrupt();

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

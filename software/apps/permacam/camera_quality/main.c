#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_power.h"
#include "nrf_twi_mngr.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_drv_spi.h"
#include "app_timer.h"
#include "app_scheduler.h"

#include <openthread/message.h>
#include <openthread/random_noncrypto.h>
#include "simple_thread.h"
#include "thread_ntp.h"
#include "thread_coap.h"
#include "thread_coap_block.h"
#include "thread_dns.h"
#include "device_id.h"
#include "flash_storage.h"

#include "permacam.h"
#include "gateway_coap.h"

#include "hm01b0.h"
#include "ab1815.h"
#include "max44009.h"
#include "HM01B0_SERIAL_FULL_8bits_msb_5fps.h"

#include "init.h"
#include "config.h"

#include "parse.pb.h"
#include "pb_encode.h"

#include "jpec.h"

APP_TIMER_DEF(camera_timer);
APP_TIMER_DEF(pir_backoff);
APP_TIMER_DEF(pir_delay);
APP_TIMER_DEF(ntp_jitter);

#define SCHED_QUEUE_SIZE 32
#define SCHED_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);
static nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);

/*
 * Permamote specific declarations for packet structure, state machine
 * ==========================
 * */
typedef struct {
  uint8_t voltage;
  uint8_t camera;
  uint8_t discover;
} permamote_sensor_period_t;
uint32_t period_count = 0;
uint32_t hours_count = 0;

static permamote_sensor_period_t sensor_period = {
  .voltage = VOLTAGE_PERIOD,
  .camera = CAMERA_PERIOD,
};

#define NUM_QUALITIES 7

#define OFFSET 256
static uint8_t image_buffer[HM01B0_RAW_IMAGE_SIZE + OFFSET];

typedef struct{
  uint8_t* buffer;
  uint8_t* raw_image;
  const uint8_t* jpeg;
  size_t   len;
  jpec_enc_t* jpeg_state;
  int8_t jpeg_quality;
  uint8_t qualities[NUM_QUALITIES];
  uint8_t current_quality;
  uint16_t current_image_id;
  struct timeval time_sent;
  float exposure;
  float sensed_lux;
  bool send_light;
  bool send_motion;
  bool send_periodic;
  bool sending_pic;
  bool update_time;
  bool update_time_wait;
  bool update_time_done;
  bool resolve_addr;
  bool resolve_wait;
  bool dfu_trigger;
  bool performing_dfu;
  bool do_reset;
} permacam_state_t;

static uint8_t dfu_jitter_hours = 0;
static bool seed = false;
uint8_t device_id[6];
permacam_state_t state = {0};

static block_info b_info = {0};

uint8_t enables[7] = {
   HM01B0_ENn,
   MAX44009_EN,
   PIR_EN,
   I2C_SDA,
   I2C_SCL
};


/*
 * ntp and coap endpoint addresses, to be populated by DNS
 * ========================================================
 * */
static otIp6Address unspecified_ipv6 = {0};
static otIp6Address m_ntp_address;
static otIp6Address m_coap_address;
static otIp6Address m_up_address;
#define NUM_ADDRESSES 3
static otIp6Address* addresses[NUM_ADDRESSES] = {&m_ntp_address, &m_coap_address, &m_up_address};

void send_jpeg();

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

void take_picture();

static void send_version(void) {
  Message msg = Message_init_default;
  strncpy(msg.data.git_version, GIT_VERSION, sizeof(msg.data.git_version));

  NRF_LOG_INFO("Sent version");

  gateway_coap_send(&m_coap_address, "git_version", false, &msg);
}

static void send_time_diff(struct timeval tv) {
  Message msg = Message_init_default;
  msg.data.time_to_send_s = tv.tv_sec;
  msg.data.time_to_send_us = tv.tv_usec;
  msg.data.image_id = state.current_image_id;
  msg.data.image_jpeg_quality = state.current_quality;
  NRF_LOG_INFO("Time to send: %ld:%d", msg.data.time_to_send_s, msg.data.time_to_send_us);

  gateway_coap_send(&m_coap_address, "time_to_send_image", false, &msg);
}

bool write_jpeg_bytes(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, state.jpeg, state.len);
}

bool write_raw_bytes(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, state.raw_image, state.len);
}

void __attribute__((weak)) thread_state_changed_callback(uint32_t flags, void * p_context) {
    NRF_LOG_INFO("State changed! Flags: 0x%08lx Current role: %d",
                 flags, otThreadGetDeviceRole(p_context));

    if (flags & OT_CHANGED_IP6_ADDRESS_ADDED && otThreadGetDeviceRole(p_context) == 2) {
      NRF_LOG_INFO("We have internet connectivity!");
      addresses_print(p_context);
      state.update_time = true;
    }
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
  state.update_time_wait = false;
  state.update_time_done = true;
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
    state.update_time = true;
}

void periodic_sensor_read_callback(void* m){
    state.send_periodic = true;
}

void light_sensor_read_callback(float lux) {
  state.sensed_lux = lux;
  state.send_light = true;
}

void send_light() {
  Message msg = Message_init_default;
  float upper = state.sensed_lux + state.sensed_lux * 0.1;
  float lower = state.sensed_lux - state.sensed_lux * 0.1;
  NRF_LOG_INFO("Sensed: lux: %u, upper: %u, lower: %u", (uint32_t)state.sensed_lux, (uint32_t)upper, (uint32_t)lower);
  max44009_set_upper_threshold(upper);
  max44009_set_lower_threshold(lower);

  msg.data.light_lux = state.sensed_lux;
  gateway_coap_send(&m_coap_address, "light_lux", false, &msg);
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


void timer_init(void) {
  APP_SCHED_INIT(SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
  ret_code_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&camera_timer, APP_TIMER_MODE_REPEATED, periodic_sensor_read_callback);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_start(camera_timer, SENSOR_PERIOD, NULL);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&pir_backoff, APP_TIMER_MODE_SINGLE_SHOT, pir_backoff_callback);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_create(&pir_delay, APP_TIMER_MODE_SINGLE_SHOT, pir_enable_callback);
  APP_ERROR_CHECK(err_code);
  err_code = app_timer_create(&ntp_jitter, APP_TIMER_MODE_SINGLE_SHOT, ntp_jitter_callback);
  APP_ERROR_CHECK(err_code);
}


void picture_sent_callback(uint8_t code, otError result) {
    NRF_LOG_INFO("DONE!");

    // send a "time_to_send" packet
    struct timeval done_time = ab1815_get_time_unix();
    struct timeval time_diff;
    time_diff.tv_sec = state.time_sent.tv_sec - done_time.tv_sec;
    time_diff.tv_usec = state.time_sent.tv_usec - done_time.tv_usec;
    memset(&state.time_sent, 0, sizeof(struct timeval));
    send_time_diff(time_diff);

    // free state pointers
    if (state.current_quality >= NUM_QUALITIES) {
      NRF_LOG_INFO("SUPER DONE!");
      otLinkSetPollPeriod(thread_get_instance(), DEFAULT_POLL_PERIOD);
      //free(state.buffer);
      //state.buffer = NULL;
      state.raw_image = NULL;
      state.current_quality = 0;
      state.jpeg_quality = state.qualities[0];
      state.sending_pic = false;
      return;
    }

    state.len = 0;
    state.jpeg_quality = state.qualities[state.current_quality++];

    if (state.current_quality >= NUM_QUALITIES) {
      // send raw!
      state.len = HM01B0_FULL_FRAME_IMAGE_SIZE;
      NRF_LOG_INFO("SEND RAW");
      NRF_LOG_INFO("raw: location %x, size %x", state.raw_image, state.len);
      static Message msg = Message_init_default;
      pb_callback_t callback;
      callback.funcs.encode = write_raw_bytes;
      msg.data.image_raw = callback;
      msg.data.image_ev = 0;
      msg.data.image_exposure_time = state.exposure;
      msg.data.image_id = state.current_image_id;
      msg.data.image_is_demosaiced = false;

      memset(&b_info, 0, sizeof(b_info));

      const char* path = "image_raw";
      memcpy(b_info.path, path, strnlen(path, sizeof(b_info.path)));
      b_info.data_len = state.len;
      b_info.code = OT_COAP_CODE_PUT;
      b_info.block_size = OT_COAP_BLOCK_SIZE_512;
      b_info.etag = otRandomNonCryptoGetUint32();

      state.time_sent = ab1815_get_time_unix();
      otLinkSetPollPeriod(thread_get_instance(), RECV_POLL_PERIOD);
      int error = gateway_coap_block_send(&m_coap_address, &b_info, &msg, picture_sent_callback, state.buffer);
      APP_ERROR_CHECK(error);
    }
    else {
      send_jpeg();
    }
}

void send_jpeg() {
    // Compress and encode as jpeg
    NRF_LOG_INFO("JPEG COMPRESS");
    NRF_LOG_INFO("SEND JPEG IMAGE, quality: %d", state.jpeg_quality);
    state.jpeg_state = jpec_enc_new2(state.raw_image, 320, 320, state.jpeg_quality);
    state.jpeg = jpec_enc_run(state.jpeg_state, (int*) &state.len);
    NRF_LOG_INFO("jpeg: location %x, size %x", state.jpeg, state.len);

    // Send picture to endpoint
    state.sending_pic = true;
    static Message msg = Message_init_default;
    pb_callback_t callback;
    callback.funcs.encode = write_jpeg_bytes;
    msg.data.image_jpeg = callback;
    msg.data.image_jpeg_quality = state.jpeg_quality;
    msg.data.image_ev = 0;
    msg.data.image_exposure_time = state.exposure;
    msg.data.image_id = state.current_image_id;
    msg.data.image_is_demosaiced = false;

    memset(&b_info, 0, sizeof(b_info));

    const char* raw_path = "image_jpeg";
    memcpy(b_info.path, raw_path, strnlen(raw_path, sizeof(b_info.path)));
    b_info.data_len = state.len;
    b_info.code = OT_COAP_CODE_PUT;
    b_info.block_size = OT_COAP_BLOCK_SIZE_512;
    b_info.etag = otRandomNonCryptoGetUint32();


    state.time_sent = ab1815_get_time_unix();
    otLinkSetPollPeriod(thread_get_instance(), RECV_POLL_PERIOD);
    int error = gateway_coap_block_send(&m_coap_address, &b_info, &msg, picture_sent_callback, NULL);
    APP_ERROR_CHECK(error);

    jpec_enc_del(state.jpeg_state);
    state.jpeg_state = NULL;
}

void take_picture() {
    int error = NRF_SUCCESS;

    if (state.sending_pic) {
      NRF_LOG_INFO("waiting to finish sending picture");
      return;
    }


    state.raw_image = state.buffer + OFFSET;
    NRF_LOG_INFO("turning on camera");
    NRF_LOG_INFO("address of buffer: %x", state.raw_image);
    NRF_LOG_INFO("size of buffer:    %x", HM01B0_RAW_IMAGE_SIZE);

    nrf_gpio_pin_clear(LED_1);
    hm01b0_power_up();

    error = hm01b0_init_system(sHM01B0InitScript, sizeof(sHM01B0InitScript)/sizeof(hm_script_t));
    NRF_LOG_INFO("error: %d", error);

    error = hm01b0_wait_for_autoexposure();
    if (error == NRF_ERROR_TIMEOUT) {
      NRF_LOG_INFO("AUTO EXPOSURE TIMEOUT");
    }

    uint16_t pck, integration_time;
    hm01b0_get_line_pck_length(&pck);
    hm01b0_get_integration(&integration_time);
    state.exposure = hm01b0_get_exposure_time(integration_time, pck);
    state.current_image_id = otRandomNonCryptoGetUint16();

    error = hm01b0_blocking_read_oneframe(state.raw_image, HM01B0_RAW_IMAGE_SIZE);
    APP_ERROR_CHECK(error);
    //image_buffer = realloc(image_buffer, offset + HM01B0_FULL_FRAME_IMAGE_SIZE);
    //if(!image_buffer) {
    //  NRF_LOG_INFO("failed to reallocate!");
    //  return;
    //}
    nrf_gpio_pin_set(LED_1);
    hm01b0_power_down();
    NRF_LOG_INFO("TOOK PICTURE");

    //if (state.jpeg_quality == 0) {
    //  // Send raw image
    //  NRF_LOG_INFO("SEND RAW IMAGE");
    //  state.buffer = image_buffer;
    //  state.len = HM01B0_FULL_FRAME_IMAGE_SIZE;

    //  msg.data.image_raw = callback;
    //  const char* raw_path = "image_raw";
    //  memcpy(b_info.path, raw_path, strnlen(raw_path, sizeof(b_info.path)));
    //}
    send_jpeg();
    state.current_quality++;
}

void state_step(void) {
  otInstance * thread_instance = thread_get_instance();
  if (state.send_light) {
    state.send_light = false;
    send_light();
  }
  if (state.send_motion) {
    state.send_motion = false;
    send_motion();
  }
  if (state.send_periodic) {
    ab1815_tickle_watchdog();

    period_count ++;
    if (period_count % sensor_period.voltage == 0) {
      //send_voltage();
    }
    if (period_count % sensor_period.camera == 0) {
      max44009_schedule_read_lux();
      take_picture();
    }
    if (period_count % sensor_period.discover == 0) {
      //send_thread_info();
    }

    //if (state.dfu_trigger == true && state.performing_dfu == false) {
    //  state.dfu_trigger = false;
    //  state.performing_dfu = true;

    //  // Send discovery packet and version before updating
    //  send_discover();
    //  send_version();

    //  otLinkSetPollPeriod(thread_instance, DFU_POLL_PERIOD);
    //  coap_remote_t remote;
    //  memcpy(&remote.addr, &m_up_address, OT_IP6_ADDRESS_SIZE);
    //  remote.port_number = OT_DEFAULT_COAP_PORT;
    //  ret_code_t err_code = app_timer_start(dfu_monitor, DFU_MONITOR_PERIOD, NULL);
    //  APP_ERROR_CHECK(err_code);
    //  int result = coap_dfu_trigger(&remote);
    //  NRF_LOG_INFO("result: %d", result);
    //  if (result == NRF_ERROR_INVALID_STATE) {
    //      coap_dfu_reset_state();
    //      nrf_dfu_settings_progress_reset();
    //  }
    //}

    state.send_periodic = false;
  }
  if (state.update_time) {
    state.update_time = false;
    state.update_time_wait = true;
    NRF_LOG_INFO("RTC UPDATE");
    otLinkSetPollPeriod(thread_instance, RECV_POLL_PERIOD);

    // resolve dns if needed
    if (!are_addresses_resolved()) {
      state.resolve_addr = true;
    }
    else {
      // dns is resolved!
      // request ntp time update
      otError error = thread_ntp_request(thread_instance, &m_ntp_address, NULL, ntp_response_handler);
      NRF_LOG_INFO("sent ntp poll!");
      NRF_LOG_INFO("error: %d", error);
      uint32_t poll = otLinkGetPollPeriod(thread_get_instance());
      if (poll != RECV_POLL_PERIOD) {
        NRF_LOG_INFO("poll: %u", poll);
        otLinkSetPollPeriod(thread_instance, RECV_POLL_PERIOD);
      }

    }
  }
  if (state.update_time_done) {
    state.update_time_done = false;
    // if we haven't performed initial setup of rand
    if (!seed) {
      srand((uint32_t)time & UINT32_MAX);
      // set jitter for next dfu check
      dfu_jitter_hours = (int)(rand() / (float) RAND_MAX * DFU_CHECK_HOURS);
      NRF_LOG_INFO("JITTER: %u", dfu_jitter_hours);
      //send_thread_info();
      send_version();
      seed = true;
    }
  }
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
    bool resolved = are_addresses_resolved();
    if (resolved)
    {
      state.update_time = true;
      state.resolve_wait = false;
    }
  }
  // if not performing a dfu, return poll period to default
  if (!state.update_time_wait && !state.resolve_wait && !state.performing_dfu && !state.sending_pic) {
    otLinkSetPollPeriod(thread_get_instance(), DEFAULT_POLL_PERIOD);
  }

  if (state.do_reset) {
    static volatile int i = 0;
    if (i++ > 100) {
      NVIC_SystemReset();
    }
  }
}

int main(void) {
    // Initialize.
    nrf_power_dcdcen_set(1);
    log_init();
    flash_storage_init();
    twi_init(&twi_mngr_instance);
    timer_init();

    sensors_init(&twi_mngr_instance, &spi_instance);

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
    thread_coap_client_init(thread_get_instance());

    // enable interrupts for pir, light sensor
    ret_code_t err_code = app_timer_start(pir_delay, PIR_DELAY, NULL);
    APP_ERROR_CHECK(err_code);
    max44009_schedule_read_lux();
    max44009_enable_interrupt();

    state.qualities[0] = 30;
    state.qualities[1] = 50;
    state.qualities[2] = 70;
    state.qualities[3] = 80;
    state.qualities[4] = 90;
    state.qualities[5] = 93;
    state.qualities[6] = 0;
    state.current_quality = 0;
    state.jpeg_quality = state.qualities[0];

    state.buffer = image_buffer;

    // Enter main loop.
    while (1) {
        thread_process();
        app_sched_execute();
        state_step();
        if (NRF_LOG_PROCESS() == false)
        {
          thread_sleep();
        }
    }
}


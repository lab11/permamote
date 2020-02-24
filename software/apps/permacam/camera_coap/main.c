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
#include "simple_thread.h"
#include "thread_ntp.h"
#include "thread_coap.h"
#include "thread_coap_block.h"
#include "thread_dns.h"
#include "device_id.h"
#include "flash_storage.h"

#include "permacam.h"
#include "permamote_coap.h"

#include "hm01b0.h"
#include "ab1815.h"
#include "HM01B0_SERIAL_FULL_8bits_msb_5fps.h"

#include "init.h"
#include "config.h"

#include "parse.pb.h"
#include "pb_encode.h"

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
  .discover = DISCOVER_PERIOD,
};

typedef struct{
  uint8_t* ptr;
  bool send_light;
  bool send_motion;
  bool send_periodic;
  bool has_pic;
  bool sending_pic;
  bool update_time;
  bool up_time_done;
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

static void send_discover(void) {
  Message msg = Message_init_default;
  strncpy(msg.data.discovery, PARSE_ADDR, sizeof(msg.data.discovery));

  NRF_LOG_INFO("Sent discovery");

  permamote_coap_send(&m_coap_address, "discovery", DEVICE_TYPE, false, &msg);
}

static void send_version(void) {
  Message msg = Message_init_default;
  strncpy(msg.data.git_version, GIT_VERSION, sizeof(msg.data.git_version));

  NRF_LOG_INFO("Sent version");

  permamote_coap_send(&m_coap_address, "version", DEVICE_TYPE, false, &msg);
}

bool write_image_bytes(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, state.ptr+256, HM01B0_IMAGE_SIZE);
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
  state.up_time_done = true;
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

void picture_sent_callback(uint8_t* data, size_t len) {
    NRF_LOG_INFO("DONE!");
    state.sending_pic = false;
    otLinkSetPollPeriod(thread_get_instance(), DEFAULT_POLL_PERIOD);
}

void take_picture() {
    if (state.sending_pic) {
      NRF_LOG_INFO("waiting to finish sending picture");
      return;
    }
    if (state.has_pic) {
      free(state.ptr);
    }
    state.ptr = malloc(HM01B0_IMAGE_SIZE + 256);
    NRF_LOG_INFO("turning on camera");
    NRF_LOG_INFO("address of buffer: %x", state.ptr + 256);
    NRF_LOG_INFO("size of buffer:    %x", HM01B0_IMAGE_SIZE);


    nrf_gpio_pin_clear(LED_1);
    hm01b0_power_up();

    int error = hm01b0_init_system(sHM01B0InitScript, sizeof(sHM01B0InitScript)/sizeof(hm_script_t));
    NRF_LOG_INFO("error: %d", error);

    hm01b0_blocking_read_oneframe(state.ptr + 256, HM01B0_IMAGE_SIZE);
    nrf_gpio_pin_set(LED_1);
    hm01b0_power_down();
    state.has_pic = true;

    // Send picture to endpoint
    state.sending_pic = true;

    Message msg = Message_init_default;
    pb_callback_t callback;
    callback.funcs.encode = write_image_bytes;
    msg.data.image_raw = callback;

    Header header = Header_init_default;
    header.version = 1;
    const char* device_type = "Permacam";
    memcpy(header.id.bytes, device_id, sizeof(device_id));
    strncpy(header.device_type, device_type, sizeof(header.device_type));
    header.id.size = sizeof(device_id);
    struct timeval time = ab1815_get_time_unix();
    header.tv_sec = time.tv_sec;
    header.tv_usec = time.tv_usec;
    header.seq_no = 0;

    memcpy(&(msg.header), &header, sizeof(header));

    pb_ostream_t stream;
    stream = pb_ostream_from_buffer(state.ptr, HM01B0_IMAGE_SIZE+ 256);
    pb_encode(&stream, Message_fields, &msg);
    size_t len = stream.bytes_written;
    NRF_LOG_INFO("len: 0x%x", len);

    memset(&b_info, 0, sizeof(b_info));
    const char* path = "image_raw";
    memcpy(b_info.path, path, strnlen(path, sizeof(b_info.path)));
    b_info.code = OT_COAP_CODE_PUT;
    b_info.data_addr = state.ptr;
    b_info.data_len = len;
    b_info.block_size = OT_COAP_BLOCK_SIZE_512;
    b_info.callback = picture_sent_callback;

    otLinkSetPollPeriod(thread_get_instance(), RECV_POLL_PERIOD);

    start_blockwise_transfer(thread_get_instance(), &m_coap_address, &b_info, block_response_handler);
}

void state_step(void) {
  otInstance * thread_instance = thread_get_instance();
  //if (state.send_light) {
  //  state.send_light = false;
  //  send_light();
  //}
  //if (state.send_motion) {
  //  state.send_motion = false;
  //  send_motion();
  //}
  if (state.send_periodic) {
    ab1815_tickle_watchdog();

    period_count ++;
    if (period_count % sensor_period.voltage == 0) {
      //send_voltage();
    }
    if (period_count % sensor_period.camera == 0) {
      //max44009_schedule_read_lux();
      take_picture();
    }
    if (period_count % sensor_period.discover == 0) {
      send_discover();
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
  if (state.up_time_done) {
    // if not performing a dfu, return poll period to default
    if (!state.performing_dfu && !state.sending_pic) {
      otLinkSetPollPeriod(thread_get_instance(), DEFAULT_POLL_PERIOD);
      state.up_time_done = false;
    }
    // if we haven't performed initial setup of rand
    if (!seed) {
      srand((uint32_t)time & UINT32_MAX);
      // set jitter for next dfu check
      dfu_jitter_hours = (int)(rand() / (float) RAND_MAX * DFU_CHECK_HOURS);
      NRF_LOG_INFO("JITTER: %u", dfu_jitter_hours);
      // send version and discover because why not
      send_discover();
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


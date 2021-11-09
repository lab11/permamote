#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "nrf_gpio.h"
#include "nrf_power.h"
#include "nrf_twi_mngr.h"
#include "nrfx_twis.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_drv_spi.h"
#include "app_timer.h"
#include "app_scheduler.h"

#include "permacam.h"

#include "hm01b0.h"

#include "jpec.h"

#define SCHED_QUEUE_SIZE 32
#define SCHED_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE

nrfx_twis_t twis_instance = NRFX_TWIS_INSTANCE(0);

//extern uint8_t image_raw[160*160];

uint8_t result[256];
uint8_t jpeg[32768];
uint32_t ind = 0;
uint32_t jpeg_size;
bool downloading_image = 0;

unsigned int crc32b(uint8_t * data, size_t len) {
   int j;
   unsigned int byte, crc, mask;

   crc = 0xFFFFFFFF;
   for (size_t i = 0; i < len; i++) {
      byte = data[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
   }
   return ~crc;
}

void twis_evt_handler(nrfx_twis_evt_t const * p_event) {
  //NRF_LOG_INFO("TWIS interrupt! %lu", p_event->type);
  if(p_event->type == NRFX_TWIS_EVT_WRITE_REQ) {
    nrfx_twis_rx_prepare(&twis_instance, result, sizeof(result));
  }
  if(p_event->type == NRFX_TWIS_EVT_WRITE_DONE) {
    size_t len = p_event->data.rx_amount;
    if (result[0] == 0x57) {
      // size packet!
      ind = 0;
      jpeg_size = *((uint32_t *) (result + 1));
      NRF_LOG_INFO("START JPEG DOWNLOAD!");
      downloading_image = true;
      memset(jpeg, 0x0, sizeof(jpeg));
    } else if (result[0] == 0x58 && downloading_image) {
      //NRF_LOG_INFO("ind: %d, len: %d", ind, len);
      //NRF_LOG_INFO("JPEG DOWNLOAD %d!", ind);
      memcpy(jpeg + ind, result + 1, len - 1);
      ind += len - 1;
    }

    if (ind >= jpeg_size && downloading_image) {
      downloading_image = false;
      NRF_LOG_INFO("DONE!");
      uint32_t crc = crc32b(jpeg, jpeg_size);
      NRF_LOG_INFO("Result: %p, size: %d, %x, crc: %lx", jpeg, jpeg_size, jpeg_size, crc);
      //nrfx_twis_disable(&twis_instance);
    }
    //nrfx_twis_disable(&twis_instance);
  }
}

void twis_init() {
  ret_code_t err_code;

  const nrfx_twis_config_t twis_config = {
    .addr = {120, 0},
    .scl  = 26,
    .sda  = 27,
    .scl_pull = NRF_GPIO_PIN_PULLUP,
    .sda_pull = NRF_GPIO_PIN_PULLUP,
    .interrupt_priority = NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY,
  };


  err_code = nrfx_twis_init(&twis_instance, &twis_config, twis_evt_handler);
  APP_ERROR_CHECK(err_code);
}

void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void) {
    //uint8_t* image_buffer = malloc(HM01B0_RAW_IMAGE_SIZE);

    // Initialize.
    nrf_power_dcdcen_set(1);
    log_init();

    APP_SCHED_INIT(SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    twis_init();


    NRF_LOG_INFO("Initialized!");

    nrfx_twis_enable(&twis_instance);
    //nrf_gpio_cfg_output(LED_1);
    //nrf_gpio_cfg_output(LED_2);
    //nrf_gpio_cfg_output(LED_3);
    //nrf_gpio_pin_set(LED_1);
    //nrf_gpio_pin_set(LED_2);
    //nrf_gpio_pin_set(LED_3);
    //for (int i = 0; i < 7; i++) {
    //  nrf_gpio_cfg_output(enables[i]);
    //  nrf_gpio_pin_set(enables[i]);
    //}

    //twi_init(&twi_mngr_instance);

    // setup RTC
    //ab1815_init(&spi_instance);
    //ab1815_control_t ab1815_config;
    //ab1815_get_config(&ab1815_config);
    //ab1815_config.auto_rst = 1;
    //ab1815_set_config(ab1815_config);

    //NRF_LOG_INFO("turning on camera");
    //NRF_LOG_INFO("address of buffer: %x", image_buffer);
    //NRF_LOG_INFO("size of buffer:    %x", HM01B0_RAW_IMAGE_SIZE);

    //hm01b0_init_i2c(&twi_mngr_instance);
    //hm01b0_mclk_init();

    //nrf_gpio_pin_clear(LED_1);
    //hm01b0_power_up();

    //int error = hm01b0_init_system(sHM01B0InitScript, sizeof(sHM01B0InitScript)/sizeof(hm_script_t));
    //NRF_LOG_INFO("error: %d", error);

    ////nrf_delay_ms(1000);
    //hm01b0_wait_for_autoexposure();

    //hm01b0_blocking_read_oneframe(image_buffer, HM01B0_RAW_IMAGE_SIZE);
    //nrf_gpio_pin_set(LED_1);
    //hm01b0_power_down();

    // Downsample the image
    //downsample_160(image_buffer);
    //image_buffer = realloc(image_buffer, 160*160);

    // Compress and encode as jpeg
    // jpec_enc_t *e = jpec_enc_new(image_buffer, 320, 320);

    //int len;
    //const uint8_t *jpeg = jpec_enc_run(e, &len);
    //NRF_LOG_INFO("jpeg location: %p", jpeg);
    //NRF_LOG_INFO("length: 0x%x", len);

    // Enter main loop.
    while (1) {
        //led_toggle(LED);
        if (NRF_LOG_PROCESS() == false)
        {
            __WFI();
        }
    }
}

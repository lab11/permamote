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
#include "app_timer.h"
#include "app_scheduler.h"

#include "permacam.h"
#include "grideye.h"

#define SCHED_QUEUE_SIZE 32
#define SCHED_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE

uint16_t pixels[8*8];

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 1);

void twi_init(const nrf_twi_mngr_t * twi_instance) {
  ret_code_t err_code;

  const nrf_drv_twi_config_t twi_config = {
    .scl                = 27,
    .sda                = 26,
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

    APP_SCHED_INIT(SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    memset(pixels, 0xde, 64*2);

    twi_init(&twi_mngr_instance);

    grideye_init(&twi_mngr_instance);
    grideye_config(GRIDEYE_PCLT_NORMAL, GRIDEYE_FPSC_10FPS);

    // Enter main loop.
    while (1) {
        //led_toggle(LED);
        if (NRF_LOG_PROCESS() == false)
        {
          //__WFI();
        }
        nrf_delay_ms(100);
        grideye_read_pixels(pixels);
        for(size_t i = 0; i < 64; i++) {
          if (i % 8 == 0) { printf("\n"); }
          printf("%.1f  ", .25 * pixels[i] * 9/5 + 32);
        }
        printf("\n\n");
    }
}

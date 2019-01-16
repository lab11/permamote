#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_gpiote.h"

#include "icount.h"
#include "permamote.h"

static nrf_drv_timer_t timer = NRF_DRV_TIMER_INSTANCE(0);
void pin_dummy_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {}
void timer_dummy_handler(nrf_timer_event_t event_type, void * p_context){}

ret_code_t icount_init(void) {
  // initialize ppi:
  nrf_drv_ppi_init();
  // initialize gpiote
  if (!nrf_drv_gpiote_is_init()) {
    nrf_drv_gpiote_init();
  }

  // set up gpiote -> counter ppi
  uint32_t gpiote_event_addr;
  uint32_t count_task_addr;
  nrf_ppi_channel_t ppi_channel;
  ret_code_t err_code;

  // initialize and configure gpiote event input
  nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(0);
  err_code = nrf_drv_gpiote_in_init(SWITCH, &config, pin_dummy_handler);
  APP_ERROR_CHECK(err_code);

  // initialize and configure timer/counter event
  nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
  timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
  timer_cfg.mode = NRF_TIMER_MODE_COUNTER;

  err_code = nrf_drv_timer_init(&timer, &timer_cfg, timer_dummy_handler);
  APP_ERROR_CHECK(err_code);

  // configure event/task and ppi channel
  err_code = nrf_drv_ppi_channel_alloc(&ppi_channel);
  APP_ERROR_CHECK(err_code);

  gpiote_event_addr = nrf_drv_gpiote_in_event_addr_get(SWITCH);
  count_task_addr   = nrf_drv_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT);

  err_code = nrf_drv_ppi_channel_assign(ppi_channel, gpiote_event_addr, count_task_addr);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_ppi_channel_enable(ppi_channel);
  APP_ERROR_CHECK(err_code);

  return err_code;
}

void icount_enable(void) {
  // enable gpio event
  nrf_drv_gpiote_in_event_enable(SWITCH, false);
  // enable timer
  nrf_drv_timer_enable(&timer);
}

void icount_disable(void) {
  // disable gpio event
  nrf_drv_gpiote_in_event_disable(SWITCH);
  // disable timer
  nrf_drv_timer_disable(&timer);
}

void icount_pause(void) {
  nrfx_timer_pause(&timer);
}

void icount_resume(void) {
  nrfx_timer_resume(&timer);
}

void icount_clear(void) {
  nrf_drv_timer_clear(&timer);
}

uint32_t icount_get_count(void) {
  nrf_drv_timer_capture(&timer, (nrf_timer_cc_channel_t)0);
  return nrf_drv_timer_capture_get(&timer, (nrf_timer_cc_channel_t)0);
}


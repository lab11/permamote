#include <math.h>

#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_rtc.h"

#include "icount.h"
#include "permamote.h"

#define FREQUENCY 32768
#define PRESCALAR 32
#define PERIOD (((float)PRESCALAR + 1) / FREQUENCY)

float * voltage_p;
size_t period;
static nrf_drv_timer_t counter = NRF_DRV_TIMER_INSTANCE(0);
static nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(0);
void pin_dummy_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {}
void timer_dummy_handler(nrf_timer_event_t event_type, void * p_context){}

static float counts_to_avg_current(float counts);

void icount_rtc_handler(nrf_drv_rtc_int_type_t type) {
  uint32_t count = nrf_drv_timer_capture_get(&counter, (nrf_timer_cc_channel_t)0);
  printf("count: %lu\n\r", count);
  printf("current: %f\n\r\n\r", counts_to_avg_current(count));
}

ret_code_t icount_init(size_t p_ms, float* v_p) {
  period = (size_t) round((float)p_ms / 1000 * FREQUENCY / (PRESCALAR + 1));
  printf("period_ticks = %u\n\r", period);
  voltage_p = v_p;

  // initialize ppi:
  nrf_drv_ppi_init();
  // initialize gpiote
  if (!nrf_drv_gpiote_is_init()) {
    nrf_drv_gpiote_init();
  }

  // set up gpiote -> counter ppi
  uint32_t gpiote_event_addr;
  uint32_t count_task_addr;
  uint32_t compare_event_addr;
  uint32_t capture_task_addr;
  uint32_t counter_clear_task_addr;
  uint32_t rtc_clear_task_addr;
  nrf_ppi_channel_t ppi_channel_count;
  nrf_ppi_channel_t ppi_channel_capture;
  nrf_ppi_channel_t ppi_channel_clear;
  ret_code_t err_code;

  // initialize and configure gpiote
  nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(0);
  err_code = nrf_drv_gpiote_in_init(SWITCH, &config, pin_dummy_handler);
  APP_ERROR_CHECK(err_code);

  // initialize and configure counter
  nrf_drv_timer_config_t counter_cfg= NRF_DRV_TIMER_DEFAULT_CONFIG;
  counter_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
  counter_cfg.mode = NRF_TIMER_MODE_COUNTER;
  err_code = nrf_drv_timer_init(&counter, &counter_cfg, timer_dummy_handler);
  APP_ERROR_CHECK(err_code);

  // initialize and configure timer
  nrf_drv_rtc_config_t rtc_cfg = NRF_DRV_RTC_DEFAULT_CONFIG;
  rtc_cfg.prescaler = PRESCALAR;
  rtc_cfg.reliable = true;
  err_code = nrf_drv_rtc_init(&rtc, &rtc_cfg, icount_rtc_handler);
  APP_ERROR_CHECK(err_code);

  // configure gpio --> counter ppi capture/clear channel and enable
  err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_count);
  APP_ERROR_CHECK(err_code);

  gpiote_event_addr = nrf_drv_gpiote_in_event_addr_get(SWITCH);
  count_task_addr   = nrf_drv_timer_task_address_get(&counter, NRF_TIMER_TASK_COUNT);

  err_code = nrf_drv_ppi_channel_assign(ppi_channel_count, gpiote_event_addr, count_task_addr);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_ppi_channel_enable(ppi_channel_count);
  APP_ERROR_CHECK(err_code);

  // configure rtc --> capture ppi channel and enable
  err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_capture);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_clear);
  APP_ERROR_CHECK(err_code);

  compare_event_addr = nrf_drv_rtc_event_address_get(&rtc, NRF_RTC_EVENT_COMPARE_0);
  capture_task_addr = nrf_drv_timer_task_address_get(&counter, NRF_TIMER_TASK_CAPTURE0);
  counter_clear_task_addr = nrf_drv_timer_task_address_get(&counter, NRF_TIMER_TASK_CLEAR);
  rtc_clear_task_addr = nrf_drv_rtc_task_address_get(&rtc, NRF_RTC_TASK_CLEAR);

  err_code = nrf_drv_ppi_channel_assign(ppi_channel_capture, compare_event_addr, capture_task_addr);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_ppi_channel_enable(ppi_channel_capture);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_drv_ppi_channel_assign(ppi_channel_clear, compare_event_addr, counter_clear_task_addr);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_ppi_channel_fork_assign(ppi_channel_clear, rtc_clear_task_addr);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_ppi_channel_enable(ppi_channel_clear);
  APP_ERROR_CHECK(err_code);

  nrf_drv_rtc_cc_set(&rtc, 0, period, true);

  return err_code;
}

void icount_enable(void) {
  // enable gpio event
  nrf_drv_gpiote_in_event_enable(SWITCH, false);
  // enable timer
  nrf_drv_timer_enable(&counter);
  nrf_drv_rtc_enable(&rtc);
}

void icount_disable(void) {
  // disable gpio event
  nrf_drv_gpiote_in_event_disable(SWITCH);
  // disable timer
  nrf_drv_timer_disable(&counter);
  nrf_drv_rtc_disable(&rtc);
}

void icount_pause(void) {
}

void icount_resume(void) {
}

void icount_clear(void) {
}

uint32_t icount_get_count(void) {
  nrf_drv_timer_capture(&counter, (nrf_timer_cc_channel_t)0);
  return nrf_drv_timer_capture_get(&counter, (nrf_timer_cc_channel_t)0);
}

static uint8_t get_closest_voltage(float v) {
    size_t x = 0;
    float dist = fabs(voltages[0] - v);
    for (size_t i = 1; i < NUM_VOLTAGES; ++i) {
        if (fabs(voltages[i] - v) < dist) {
            x = i;
            dist = fabs(voltages[i] - v);
        }
    }
    printf("closest: %u\n\r", x);
    return x;
}

static float counts_to_avg_current(float count) {
    size_t i = get_closest_voltage(*voltage_p);
    return (count/(float)period*1000 - bias[i]/*- c_table[i*2 + 1]*/) / c_table[i*2];
}

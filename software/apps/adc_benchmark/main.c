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
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"
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

#include "permamote.h"

#include <openthread/message.h>

//#define SAADC_SAMPLE_RATE 488 // every 1/2048th of a second
#define SAADC_SAMPLE_RATE 244 // every 1/4096th of a second
#define SAADC_SAMPLES_IN_BUFFER 2
#define SAADC_OVERSAMPLE NRF_SAADC_OVERSAMPLE_DISABLED
#define SAADC_BURST_MODE 0

static const nrf_drv_timer_t   m_timer = NRF_DRV_TIMER_INSTANCE(3);
static nrf_saadc_value_t       m_buffer_pool[2][SAADC_SAMPLES_IN_BUFFER];
static nrf_ppi_channel_t       m_ppi_channel;
static uint32_t                m_adc_evt_counter;

static uint16_t sample_count = 0;
static float dcurrent[4096];
static float voltage[4096];

uint8_t enables[7] = {
   MAX44009_EN,
   ISL29125_EN,
   MS5637_EN,
   SI7021_EN,
   PIR_EN,
   I2C_SDA,
   I2C_SCL
};

static void lfclk_config(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}

void saadc_handler(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        ret_code_t err_code;
        uint8_t value[SAADC_SAMPLES_IN_BUFFER*2];

        // set buffers
        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAADC_SAMPLES_IN_BUFFER);
        APP_ERROR_CHECK(err_code);

        printf("%d, %d\n", p_event->data.done.p_buffer[0], p_event->data.done.p_buffer[1]);

        // print samples on hardware UART and parse data for BLE transmission
        //printf("ADC event number: %d\r\n",(int)m_adc_evt_counter);
        //for (int i = 0; i < SAADC_SAMPLES_IN_BUFFER; i++)
        //{
        //    printf("%d\r\n", p_event->data.done.p_buffer[i]);

        //    adc_value = p_event->data.done.p_buffer[i];
        //    value[i*2] = adc_value;
        //    value[(i*2)+1] = adc_value >> 8;
        //}

        //// Send data over BLE via NUS service. Makes sure not to send more than 20 bytes.
        //if((SAADC_SAMPLES_IN_BUFFER*2) <= 20)
        //{
        //    bytes_to_send = (SAADC_SAMPLES_IN_BUFFER*2);
        //}
        //else
        //{
        //    bytes_to_send = 20;
        //}
        //err_code = ble_nus_string_send(&m_nus, value, bytes_to_send);
        //if (err_code != NRF_ERROR_INVALID_STATE)
        //{
        //    APP_ERROR_CHECK(err_code);
        //}

        m_adc_evt_counter++;
    }
}

void timer_handler(nrf_timer_event_t event_type, void* p_context)
{

}

void saadc_sampling_event_init(void) {
    ret_code_t err_code;
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_config_t timer_config = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_config.frequency = NRF_TIMER_FREQ_31250Hz;
    err_code = nrf_drv_timer_init(&m_timer, &timer_config, timer_handler);
    APP_ERROR_CHECK(err_code);

    /* setup m_timer for compare event */
    uint32_t ticks = nrf_drv_timer_ms_to_ticks(&m_timer,250);
    nrf_drv_timer_extended_compare(&m_timer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
    nrf_drv_timer_enable(&m_timer);

    uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&m_timer, NRF_TIMER_CC_CHANNEL0);
    uint32_t saadc_sample_event_addr = nrf_drv_saadc_sample_task_get();

    /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
    err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(m_ppi_channel, timer_compare_event_addr, saadc_sample_event_addr);
    APP_ERROR_CHECK(err_code);
}

void saadc_sampling_event_enable(void)
{
    ret_code_t err_code = nrf_drv_ppi_channel_enable(m_ppi_channel);
    APP_ERROR_CHECK(err_code);
}

void saadc_init(void) {

    ret_code_t err_code;

    nrf_drv_saadc_config_t saadc_config = {0};
    saadc_config.low_power_mode = 1;
    saadc_config.resolution = NRF_SAADC_RESOLUTION_12BIT;

    // set up voltage ADC
    nrf_saadc_channel_config_t primary_channel_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN5);

    nrf_saadc_channel_config_t secondary_channel_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN7);

    err_code = nrf_drv_saadc_init(&saadc_config, saadc_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &primary_channel_config);
    APP_ERROR_CHECK(err_code);
    //err_code = nrf_drv_saadc_channel_init(1, &secondary_channel_config);
    //APP_ERROR_CHECK(err_code);

    // set up double buffers
    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAADC_SAMPLES_IN_BUFFER);
    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], SAADC_SAMPLES_IN_BUFFER);

    nrf_drv_saadc_calibrate_offset();
}


void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void) {
    nrf_power_dcdcen_set(1);
    lfclk_config();

    nrf_gpio_cfg_output(LED_1);
    nrf_gpio_cfg_output(LED_2);
    nrf_gpio_cfg_output(LED_3);
    nrf_gpio_pin_set(LED_1);
    nrf_gpio_pin_set(LED_2);
    nrf_gpio_pin_set(LED_3);
    for (int i = 0; i < 7; i++) {
      nrf_gpio_cfg_output(enables[i]);
      nrf_gpio_pin_set(enables[i]);
    }

    nrf_gpio_cfg_output(LI2D_CS);
    nrf_gpio_cfg_output(SPI_MISO);
    nrf_gpio_cfg_output(SPI_MOSI);
    nrf_gpio_pin_set(LI2D_CS);
    nrf_gpio_pin_set(SPI_MISO);
    nrf_gpio_pin_set(SPI_MOSI);

    // Init log
    log_init();
    //printf("\nADC TEST\n");

    saadc_sampling_event_init();
    saadc_init();
    saadc_sampling_event_enable();

    while (1) {
        if (NRF_LOG_PROCESS() == false)
        {
            __WFE();
        }
    }
}

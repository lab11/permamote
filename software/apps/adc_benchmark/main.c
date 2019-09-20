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
#include "nrf_drv_rtc.h"
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
#define SAADC_SAMPLE_FREQ 4096 // every 1/4096th of a second
#define SAADC_SAMPLES_IN_BUFFER 3
#define SAADC_OVERSAMPLE NRF_SAADC_OVERSAMPLE_DISABLED
#define SAADC_BURST_MODE 0

static const nrf_drv_rtc_t  rtc = NRF_DRV_RTC_INSTANCE(0);
static nrf_saadc_value_t    m_buffer_pool[2][SAADC_SAMPLES_IN_BUFFER];
static nrf_ppi_channel_t    m_ppi_channel;
static uint32_t             m_adc_evt_counter;

static uint16_t sample_count = 0;
static float dcurrent[SAADC_SAMPLE_FREQ];
static float voltage[SAADC_SAMPLE_FREQ];

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
        int16_t* buffer = p_event->data.done.p_buffer;

        // set buffers
        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAADC_SAMPLES_IN_BUFFER);
        APP_ERROR_CHECK(err_code);

        printf("%d, %d, %d\n", buffer[0], buffer[1], buffer[2]);
        //printf("%d\n", buffer[0]);

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

static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{

}



void saadc_sampling_event_init(void) {
    ret_code_t err_code;
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    config.reliable = 1;
    err_code = nrf_drv_rtc_init(&rtc, &config, rtc_handler);
    APP_ERROR_CHECK(err_code);

    //Set compare channel 0 to trigger at 4096 Hz
    err_code = nrf_drv_rtc_cc_set(&rtc,0,8,false);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    nrf_drv_rtc_enable(&rtc);

    uint32_t rtc_compare_event_addr = nrf_drv_rtc_event_address_get(&rtc, NRF_RTC_EVENT_COMPARE_0);
    uint32_t rtc_clear_task_addr = nrf_drv_rtc_task_address_get(&rtc, NRF_RTC_TASK_CLEAR);
    uint32_t saadc_sample_event_addr = nrf_drv_saadc_sample_task_get();

    /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
    err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(m_ppi_channel, rtc_compare_event_addr, saadc_sample_event_addr);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_fork_assign(m_ppi_channel, rtc_clear_task_addr);
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
    nrf_saadc_channel_config_t channel_config0 =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN5);

    nrf_saadc_channel_config_t channel_config1 =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN6);

    nrf_saadc_channel_config_t channel_config2 =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN7);

    err_code = nrf_drv_saadc_init(&saadc_config, saadc_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config0);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_saadc_channel_init(1, &channel_config1);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_saadc_channel_init(2, &channel_config2);
    APP_ERROR_CHECK(err_code);

    // set up double buffers
    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAADC_SAMPLES_IN_BUFFER);
    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], SAADC_SAMPLES_IN_BUFFER);

    //nrf_drv_saadc_calibrate_offset();
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
    printf("\nADC TEST\n");

    saadc_init();
    saadc_sampling_event_init();
    saadc_sampling_event_enable();

    while (1) {
        if (NRF_LOG_PROCESS() == false)
        {
            __WFI();
        }
    }
}

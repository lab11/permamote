#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_util.h"
#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "led.h"
#include "app_uart.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"

#include "simple_ble.h"
#include "simple_adv.h"
#include "multi_adv.h"
#include "eddystone.h"
#include "permamote.h"

#define UART_TX_BUF_SIZE     256
#define UART_RX_BUF_SIZE     256

#define LED0 NRF_GPIO_PIN_MAP(0,4)
#define LED1 NRF_GPIO_PIN_MAP(0,5)
#define LED2 NRF_GPIO_PIN_MAP(0,6)

void uart_error_handle (app_uart_evt_t * p_event) {
  if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
    APP_ERROR_HANDLER(p_event->data.error_communication);
  } else if (p_event->evt_type == APP_UART_FIFO_ERROR) {
    APP_ERROR_HANDLER(p_event->data.error_code);
  }
}

static void interrupt_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  //nrf_drv_gpiote_in_event_enable(PIR_OUT, 0);
  //if (nrf_drv_gpiote_in_is_set(pin)) {
  printf("I see you.\r\n");
  //}
  //else {
    //printf("Are you still there?\r\n");
  //}
  //nrf_drv_gpiote_in_event_enable(PIR_OUT, 1);
}

void uart_init(void) {
  uint32_t err_code;

  const app_uart_comm_params_t comm_params =
  {
    UART_RX,
    UART_TX,
    0,
    0,
    APP_UART_FLOW_CONTROL_DISABLED,
    false,
    UART_BAUDRATE_BAUDRATE_Baud115200
  };
  APP_UART_FIFO_INIT(&comm_params,
      UART_RX_BUF_SIZE,
      UART_TX_BUF_SIZE,
      uart_error_handle,
      APP_IRQ_PRIORITY_LOW,
      err_code);
  APP_ERROR_CHECK(err_code);

}

/*******************************************************************************
 *   Configure Advertisements
 ******************************************************************************/

//static void adv_config_eddystone () {
//    eddystone_adv(PHYSWEB_URL, NULL);
//}
//
//static void adv_config_data () {
//    ble_advdata_manuf_data_t manuf_specific_data;
//
//    // update sequence number only if data actually changed
//    if (data_updated) {
//        m_sensor_info.sequence_num++;
//        data_updated = false;
//    }
//
//    // Register this manufacturer data specific data as the BLEES service
//    m_beacon_info[0] = APP_BEACON_INFO_SERVICE_PERM;
//    //memcpy(&m_beacon_info[1],  &m_sensor_info.light, 4);
//    memcpy(&m_beacon_info[1],  &m_sensor_info.light_exp, 1);
//    memcpy(&m_beacon_info[2],  &m_sensor_info.light_mant, 1);
//    memcpy(&m_beacon_info[3],  &m_sensor_info.color_red, 2);
//    memcpy(&m_beacon_info[5],  &m_sensor_info.color_green, 2);
//    memcpy(&m_beacon_info[7],  &m_sensor_info.color_blue, 2);
//    memcpy(&m_beacon_info[9],  &m_sensor_info.vbat, 2);
//    memcpy(&m_beacon_info[11],  &m_sensor_info.vsol, 2);
//    memcpy(&m_beacon_info[13],  &m_sensor_info.vsec, 2);
//    memcpy(&m_beacon_info[15],  &m_sensor_info.sequence_num, 4);
//
//    memset(&manuf_specific_data, 0, sizeof(manuf_specific_data));
//    manuf_specific_data.company_identifier = UMICH_COMPANY_IDENTIFIER;
//    manuf_specific_data.data.p_data = (uint8_t*) m_beacon_info;
//    manuf_specific_data.data.size   = APP_BEACON_INFO_LENGTH;
//
//    simple_adv_manuf_data(&manuf_specific_data);
//}

/*******************************************************************************
 *   MAIN LOOP
 ******************************************************************************/

int main(void) {
  // init uart
  uart_init();

  // init softdevice
  nrf_sdh_enable_request();
  sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  //simple_ble_init(&ble_config);

  //multi_adv_init(ADV_SWITCH_MS);
  // Now register our advertisement configure functions
  //multi_adv_register_config(adv_config_eddystone);
  //multi_adv_register_config(adv_config_data);

  // init led
  led_init(LED2);
  led_off(LED2);


  printf("\r\nPIR TEST\r\n");

  // power gate
  nrf_gpio_cfg_output(MAX44009_EN);
  nrf_gpio_cfg_output(ISL29125_EN);
  nrf_gpio_cfg_output(MS5637_EN);
  nrf_gpio_cfg_output(SI7021_EN);
  nrf_gpio_cfg_output(PIR_EN);
  nrf_gpio_pin_set(MAX44009_EN);
  nrf_gpio_pin_set(ISL29125_EN);
  nrf_gpio_pin_set(MS5637_EN);
  nrf_gpio_pin_set(SI7021_EN);
  nrf_gpio_pin_clear(PIR_EN);

  nrf_delay_ms(2000);

  if (!nrf_drv_gpiote_is_init()) {
    nrf_drv_gpiote_init();
  }
  nrf_drv_gpiote_in_config_t int_gpio_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(0);
  int_gpio_config.pull = NRF_GPIO_PIN_PULLDOWN;
  nrf_drv_gpiote_in_init(PIR_OUT, &int_gpio_config, interrupt_handler);
  nrf_drv_gpiote_in_event_enable(PIR_OUT, 1);

  while (1) {
    power_manage();
  }
}

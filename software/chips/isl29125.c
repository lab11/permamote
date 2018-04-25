#include "nrf_drv_gpiote.h"

#include "isl29125.h"
#include "permamote.h"

static const nrf_twi_mngr_t* twi_mngr_instance;
static isl29125_config_t isl_config;
static isl29125_int_config_t isl_int_config;
static uint16_t range = 375;
static uint8_t resolution = 16;
static uint8_t color_data[6] = {0};
isl29125_color_read_callback_t* color_read_callback = NULL;
isl29125_interrupt_callback_t* interrupt_callback = NULL;

static void interrupt_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  isl29125_read_status();
  interrupt_callback();
}

static void isl29125_read_callback(ret_code_t result, void* p_context) {
  uint16_t red, green, blue;
  green  = (color_data[0] | (uint16_t)color_data[1] << 8);// * range / (1 << resolution);
  red    = (color_data[2] | (uint16_t)color_data[3] << 8);// * range / (1 << resolution);
  blue   = (color_data[4] | (uint16_t)color_data[5] << 8);// * range / (1 << resolution);
  color_read_callback(red, green, blue);
}

void isl29125_init(const nrf_twi_mngr_t*            instance,
                   isl29125_config_t                config,
                   isl29125_int_config_t            int_config,
                   isl29125_color_read_callback_t*  read_callback,
                   isl29125_interrupt_callback_t*   int_callback) {
  twi_mngr_instance = instance;
  isl_config = config;
  isl_int_config = int_config;
  color_read_callback = read_callback;
  interrupt_callback = int_callback;

  if (interrupt_callback != NULL) {
    // setup gpiote interrupt
    if (!nrf_drv_gpiote_is_init()) {
      nrf_drv_gpiote_init();
    }
    nrf_drv_gpiote_in_config_t int_gpio_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(0);
    int_gpio_config.pull = NRF_GPIO_PIN_NOPULL;
    nrf_drv_gpiote_in_init(ISL29125_INT, &int_gpio_config, interrupt_handler);
  }
}

uint8_t isl29125_read_status() {
  static uint8_t status_addr = ISL29125_STATUS;
  static uint8_t status;

  static nrf_twi_mngr_transfer_t const status_transfer[] =
  {
    NRF_TWI_MNGR_WRITE(ISL29125_ADDR, &status_addr, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(ISL29125_ADDR, &status, 1, 0)
  };

  nrf_twi_mngr_perform(twi_mngr_instance, status_transfer, 2, NULL);

  return status;
}

void isl29125_configure(isl29125_config_t config) {
  static uint8_t data[2];
  uint8_t config_byte = 0;
  config_byte = config.sync_int << 5 |
                config.resolution << 4 |
                config.range << 3 |
                config.mode;
  data[0] = ISL29125_CFG1;
  data[1] = config_byte;

  // configure sensor
  static nrf_twi_mngr_transfer_t const config_transfer[] =
  {
    NRF_TWI_MNGR_WRITE(ISL29125_ADDR, data, 2, 0),
  };
  nrf_twi_mngr_perform(twi_mngr_instance, config_transfer, 1, NULL);

  if (config.range) range = 10000;
  else range = 375;
  if (config.resolution) resolution = 12;
  else resolution = 16;
}

void isl29125_interrupt_configure(isl29125_int_config_t config){
  static uint8_t data[2];
  uint8_t config_byte = config.mode | config.persist << 2 |
                        config.int_conversion_done << 4;

  data[0] = ISL29125_CFG3;
  data[1] = config_byte;

  // configure interrupt settings on sensor
  static nrf_twi_mngr_transfer_t const config_transfer[] =
  {
    NRF_TWI_MNGR_WRITE(ISL29125_ADDR, data, 2, 0),
  };
  nrf_twi_mngr_perform(twi_mngr_instance, config_transfer, 2, NULL);
}

void isl29125_interrupt_enable() {
  isl29125_interrupt_configure(isl_int_config);

  if (interrupt_callback != NULL) {
    // enable interrupt trigger
    nrf_drv_gpiote_in_event_enable(ISL29125_INT, 1);
  }
}

void isl29125_interrupt_disable() {
  // disable interrupt on sensor
  isl29125_int_config_t disable_config = {0};
  isl29125_interrupt_configure(disable_config);

  if (interrupt_callback != NULL) {
    // disable interrupt trigger
    nrf_drv_gpiote_in_event_disable(ISL29125_INT);
  }
}

void isl29125_schedule_color_read(void) {
  static const uint8_t color_addr = ISL29125_G_LOW;

  static nrf_twi_mngr_transfer_t const color_read_transfer[] = {
    NRF_TWI_MNGR_WRITE(ISL29125_ADDR, &color_addr, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(ISL29125_ADDR, color_data, 6, 0)
  };

  static nrf_twi_mngr_transaction_t const color_read_transaction =
  {
    .callback = isl29125_read_callback,
    .p_user_data = NULL,
    .p_transfers = color_read_transfer,
    .number_of_transfers = sizeof(color_read_transfer)/sizeof(color_read_transfer[0]),
    .p_required_twi_cfg = NULL
  };

  APP_ERROR_CHECK(nrf_twi_mngr_schedule(twi_mngr_instance, &color_read_transaction));
}

void isl29125_power_down(void) {
  isl29125_config_t new_config = isl_config;
  new_config.mode = isl29125_mode_power_down;
  isl29125_configure(new_config);
}

void isl29125_power_on(void) {
  // clear pending interrupts
  isl29125_read_status();
  isl29125_configure(isl_config);
}

#include "max44009.h"

#include "nrf_delay.h"

static const nrf_twi_mngr_t* twi_mngr_instance;
static uint8_t lux_read_buf[2] = {0};
static uint8_t config_buf[2] = {MAX44009_CONFIG, 0};
static const uint8_t max44009_lux_addr = MAX44009_LUX_HIGH;

static max44009_read_lux_callback* master_callback;

static void max44009_lux_callback(ret_code_t result, void* p_context) {
  uint8_t exp, mantissa;
  float lux;
  exp = (lux_read_buf[0] & 0xF0) >> 4;
  mantissa = lux_read_buf[0] & 0xF0;
  mantissa |= lux_read_buf[1] & 0xF;
  lux = (float)(1 << exp) * (float)mantissa * 0.045;
  master_callback(lux);
}

static nrf_twi_mngr_transfer_t const lux_read_transfer[] = {
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, &max44009_lux_addr, 1, NRF_TWI_MNGR_NO_STOP),
  NRF_TWI_MNGR_READ(MAX44009_ADDR, lux_read_buf, 2, 0)
};

static nrf_twi_mngr_transfer_t const config_transfer[] = {
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, config_buf, 2, 0)
};

static nrf_twi_mngr_transaction_t const lux_read_transaction =
{
  .callback = max44009_lux_callback,
  .p_user_data = NULL,
  .p_transfers = lux_read_transfer,
  .number_of_transfers = sizeof(lux_read_transfer)/sizeof(lux_read_transfer[0]),
  .p_required_twi_cfg = NULL

};

static nrf_twi_mngr_transaction_t const config_transaction =
{
  .callback = NULL,
  .p_user_data = NULL,
  .p_transfers = config_transfer,
  .number_of_transfers = sizeof(config_transfer)/sizeof(config_transfer[0]),
  .p_required_twi_cfg = NULL

};

void max44009_init(const nrf_twi_mngr_t* instance, max44009_read_lux_callback* callback) {
  twi_mngr_instance = instance;
  master_callback = callback;
}

void max44009_config(max44009_config_t config) {
  uint8_t config_byte = config.continuous << 7 |
                        config.manual << 6 |
                        config.cdr << 3 |
                        (config.int_time & 0x7);

  config_buf[1] = config_byte;

  int error = (nrf_twi_mngr_schedule(twi_mngr_instance, &config_transaction));
  APP_ERROR_CHECK(error);
}

void max44009_schedule_read_lux(void) {
  APP_ERROR_CHECK(nrf_twi_mngr_schedule(twi_mngr_instance, &lux_read_transaction));
}

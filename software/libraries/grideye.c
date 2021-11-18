#include "grideye.h"

#include "assert.h"

#include "nrf_delay.h"

const nrf_twi_mngr_t * twi_mngr_instance;

static uint32_t write_register(uint8_t reg, const uint8_t* value, size_t len) {
  #define GRIDEYE_BUFFER_SIZE 16
  uint8_t buf[GRIDEYE_BUFFER_SIZE];
  assert(GRIDEYE_BUFFER_SIZE >= len + 1);
  buf[0] = reg;
  memcpy(buf+1, value, len);

  nrf_twi_mngr_transfer_t const transfer[] = {
    NRF_TWI_MNGR_WRITE(GRIDEYE_I2C_ADDR, buf, len+1, 0),
  };
  uint32_t error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, transfer, 1, NULL);

  return error;
}

uint32_t read_register(uint8_t reg, uint8_t* value, size_t len) {
  nrf_twi_mngr_transfer_t const transfer[] = {
    NRF_TWI_MNGR_WRITE(GRIDEYE_I2C_ADDR, &reg, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ (GRIDEYE_I2C_ADDR, value, len, 0),
  };
  uint32_t error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, transfer, 2, NULL);

  return error;
}

void grideye_init(const nrf_twi_mngr_t * instance) {
  twi_mngr_instance = instance;
}

void grideye_config(grideye_pclt_t pclt, grideye_fpsc_t fpsc) {
  uint8_t pclt_val = pclt;
  uint8_t fpsc_val = fpsc;
  uint8_t reset = GRIDEYE_INIT_RST;
  ret_code_t err = NRF_SUCCESS;

  err = write_register(GRIDEYE_PCLT_ADDR, &reset, sizeof(reset));
  APP_ERROR_CHECK(err);
  nrf_delay_ms(200);
  err = write_register(GRIDEYE_PCLT_ADDR, &pclt_val, sizeof(pclt_val));
  APP_ERROR_CHECK(err);
  err = write_register(GRIDEYE_FPSC_ADDR, &fpsc_val, sizeof(fpsc_val));
  APP_ERROR_CHECK(err);
}

void grideye_read_pixels(uint16_t * pixels) {
  ret_code_t err = NRF_SUCCESS;
  err = read_register(GRIDEYE_T01L_ADDR, (uint8_t*) pixels, GRIDEYE_NUM_PIXELS * sizeof(pixels[0]));
  APP_ERROR_CHECK(err);
}

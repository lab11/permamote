#include "math.h"

#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"

#include "ms5637.h"

typedef enum Pressure_OSR_enum {
  p_osr_8192  = 0x4A,
  p_osr_4096  = 0x48,
  p_osr_2048  = 0x46,
  p_osr_1024  = 0x44,
  p_osr_512   = 0x42,
  p_osr_256   = 0x40
} Pressure_OSR;
typedef enum Temperature_OSR_enum {
  t_osr_8192  = 0x5A,
  t_osr_4096  = 0x58,
  t_osr_2048  = 0x56,
  t_osr_1024  = 0x54,
  t_osr_512   = 0x52,
  t_osr_256   = 0x50
} Temperature_OSR;


static const nrf_twi_mngr_t* twi_mngr_instance = NULL;
static MS5637_OSR osr = osr_2048;
static uint8_t prom[1]  = {MS5637_PROM};
static uint8_t rst[1]   = {MS5637_RESET};
static uint8_t c_buf[2];
static uint16_t c[6];
static uint8_t conv_cmd[1]  = {0};
static uint8_t data[3] = {0};

static nrf_twi_mngr_transfer_t const reset_transfer[] = {
  NRF_TWI_MNGR_WRITE(MS5637_ADDR, rst, 1, 0),
};

static nrf_twi_mngr_transfer_t const read_prom_transfer[] = {
  NRF_TWI_MNGR_WRITE(MS5637_ADDR, prom, 1, 0),
  NRF_TWI_MNGR_READ(MS5637_ADDR, c_buf, 2, 0)
};

static nrf_twi_mngr_transfer_t const start_conversion_transfer[] = {
  NRF_TWI_MNGR_WRITE(MS5637_ADDR, conv_cmd, 1, 0),
};

static nrf_twi_mngr_transfer_t const read_adc_transfer[] = {
  NRF_TWI_MNGR_WRITE(MS5637_ADDR, data, 1, 0),
  NRF_TWI_MNGR_READ(MS5637_ADDR, data, 3, 0),
};

static inline Pressure_OSR ms5637_osr_to_posr(MS5637_OSR osr_config) {
  switch(osr_config) {
    case osr_8192: return p_osr_8192;
    case osr_4096: return p_osr_4096;
    case osr_2048: return p_osr_2048;
    case osr_1024: return p_osr_1024;
    case osr_512:  return p_osr_512;
    case osr_256:  return p_osr_256;
    default: return p_osr_256;
  }
}

static inline Temperature_OSR ms5637_osr_to_tosr(MS5637_OSR osr_config) {
  switch(osr_config) {
    case osr_8192: return t_osr_8192;
    case osr_4096: return t_osr_4096;
    case osr_2048: return t_osr_2048;
    case osr_1024: return t_osr_1024;
    case osr_512:  return t_osr_512;
    case osr_256:  return t_osr_256;
    default: return t_osr_256;
  }
}

static inline uint8_t ms5637_osr_to_delay_ms(MS5637_OSR osr_config) {
  switch(osr_config) {
    case osr_8192: return 17;
    case osr_4096: return 9;
    case osr_2048: return 5;
    case osr_1024: return 3;
    case osr_512:  return 2;
    case osr_256:  return 1;
    default: return 20;
  }
}

void ms5637_init(const nrf_twi_mngr_t* instance, MS5637_OSR osr_config) {
  twi_mngr_instance = instance;
  osr = osr_config;
}

void ms5637_start(void) {
  // Reset Sensor
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, reset_transfer, 1, NULL);
  APP_ERROR_CHECK(error);

  // Read PROM configuration
  for(int i = 0; i < 6; ++i) {
    *prom = MS5637_PROM + i*2;
    error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_prom_transfer, sizeof(read_prom_transfer)/sizeof(read_prom_transfer[0]), NULL);
    APP_ERROR_CHECK(error);
    c[i] = c_buf[0] << 8 | c_buf[1];
  }
}

uint32_t get_raw(uint8_t cmd){
  // Start Conversion
  *conv_cmd = cmd;
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, start_conversion_transfer, 1, NULL);
  APP_ERROR_CHECK(error);
  nrf_delay_ms(ms5637_osr_to_delay_ms(osr));
  // Read ADC conversion
  data[0] = 0;
  error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_adc_transfer, 2, NULL);
  APP_ERROR_CHECK(error);
  uint32_t raw = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
  return raw;
}

void ms5637_get_temperature_and_pressure(float* temp, float* pres) {
  // Get Pressure Conversion
  uint32_t raw_pres = get_raw(ms5637_osr_to_posr(osr));
  // Get Temperature Conversion
  uint32_t raw_temp = get_raw(ms5637_osr_to_tosr(osr));

  int32_t dT = (int32_t)raw_temp - ((int32_t)c[4] << 8);
  int32_t temperature = 2000 + ((int64_t)dT * (int64_t)c[5] >> 23);

  // Pressure calculation based on datasheet
  int64_t offset = (uint64_t)c[1] * 131072 + (int64_t)c[3] * dT / 64;
  int64_t sens = (int64_t)c[0] * 65536 + (int64_t)c[2] * dT / 128;
  int32_t pressure = ((uint64_t)raw_pres * sens / 2097152 - offset )/ 32768;

  *temp = temperature / 100.0;
  *pres = pressure / 100.0;
}

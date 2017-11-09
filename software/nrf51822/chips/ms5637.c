#include "math.h"

#include "nrf_delay.h"

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


static nrf_drv_twi_t* twi_instance = NULL;
static uint16_t c1 = 0;
static uint16_t c2 = 0;
static uint16_t c3 = 0;
static uint16_t c4 = 0;
static uint16_t c5 = 0;
static uint16_t c6 = 0;

static inline Pressure_OSR ms5637_osr_to_posr(OSR osr) {
  switch(osr) {
    case osr_8192: return p_osr_8192;
    case osr_4096: return p_osr_4096;
    case osr_2048: return p_osr_2048;
    case osr_1024: return p_osr_1024;
    case osr_512:  return p_osr_512;
    case osr_256:  return p_osr_256;
    default: return p_osr_256;
  }
}

static inline Temperature_OSR ms5637_osr_to_tosr(OSR osr) {
  switch(osr) {
    case osr_8192: return t_osr_8192;
    case osr_4096: return t_osr_4096;
    case osr_2048: return t_osr_2048;
    case osr_1024: return t_osr_1024;
    case osr_512:  return t_osr_512;
    case osr_256:  return t_osr_256;
    default: return t_osr_256;
  }
}

static inline void ms5637_reg_read(uint8_t reg, uint8_t* data, uint8_t len) {
  nrf_drv_twi_tx(twi_instance, MS5637_ADDR, &reg, 1, false);
  nrf_drv_twi_rx(twi_instance, MS5637_ADDR, data, len);
}

void ms5637_init(nrf_drv_twi_t* instance) {
  twi_instance = instance;
}
void ms5637_start(void) {
  uint8_t data[2] = {0};

  nrf_drv_twi_enable(twi_instance);

  // Reset Sensor
  data[0] = 0x1E;
  nrf_drv_twi_tx(twi_instance, MS5637_ADDR, data, 1, false);

  // Read PROM configuration
  ms5637_reg_read(0xA2, data, 2);
  c1 = 0xFFFF & (data[0] << 8 | data[1]);

  ms5637_reg_read(0xA4, data, 2);
  c2 = 0xFFFF & (data[0] << 8 | data[1]);

  ms5637_reg_read(0xA6, data, 2);
  c3 = 0xFFFF & (data[0] << 8 | data[1]);

  ms5637_reg_read(0xA8, data, 2);
  c4 = 0xFFFF & (data[0] << 8 | data[1]);

  ms5637_reg_read(0xAA, data, 2);
  c5 = 0xFFFF & (data[0] << 8 | data[1]);

  ms5637_reg_read(0xAC, data, 2);
  c6 = 0xFFFF & (data[0] << 8 | data[1]);

  nrf_drv_twi_disable(twi_instance);

  //printf("c1: %d\n", c1);
  //printf("c2: %d\n", c2);
  //printf("c3: %d\n", c3);
  //printf("c4: %d\n", c4);
  //printf("c5: %d\n", c5);
  //printf("c6: %d\n", c6);
}

float ms5637_get_temperature(OSR osr) {
  uint8_t data[3];

  // Start Temperature Conversion
  data[0] = (uint8_t) ms5637_osr_to_tosr(osr);
  nrf_drv_twi_enable(twi_instance);
  nrf_drv_twi_tx(twi_instance, MS5637_ADDR, data, 1, false);
  // Wait 20 msec for conversion
  nrf_delay_ms(20);
  // Read ADC conversion
  ms5637_reg_read(0x00, data, 3);
  nrf_drv_twi_disable(twi_instance);
  uint32_t raw = 0xffffff & (data[0] << 16 | data[1] << 8 | data[2]);

  // Temperature calculation based on datasheet
  // It isn't clear how the osr resolution affects these calculations
  int32_t dT = raw - c5 * 256;
  int32_t temperature = 2000 + (int64_t)dT * (int64_t)c6 / 8388608;
  return temperature / 100.0;
}

float ms5637_get_pressure(OSR osr) {
  uint8_t data[3];

  // Start Temperature Conversion
  data[0] = (uint8_t) ms5637_osr_to_tosr(osr);
  nrf_drv_twi_enable(twi_instance);
  nrf_drv_twi_tx(twi_instance, MS5637_ADDR, data, 1, false);
  // Wait 20 msec for conversion
  nrf_delay_ms(20);
  // Read ADC conversion
  ms5637_reg_read(0x00, data, 3);
  nrf_drv_twi_disable(twi_instance);
  uint32_t raw = 0xffffff & (data[0] << 16 | data[1] << 8 | data[2]);
  // Temperature calculation based on datasheet
  // It isn't clear how the osr resolution affects these calculations
  int32_t dT = raw - c5 * 256;

  // Start Pressure Conversion
  data[0] = (uint8_t) ms5637_osr_to_posr(osr);
  nrf_drv_twi_enable(twi_instance);
  nrf_drv_twi_tx(twi_instance, MS5637_ADDR, data, 1, false);
  // Wait 20 msec for conversion
  nrf_delay_ms(20);
  // Read ADC conversion
  ms5637_reg_read(0x00, data, 3);
  nrf_drv_twi_disable(twi_instance);
  raw = 0xffffff & (data[0] << 16 | data[1] << 8 | data[2]);

  // Pressure calculation based on datasheet
  int64_t offset = (int64_t)c2 * 131072 + (int64_t)c4 * (int64_t)dT / 64;
  int64_t sens = (int64_t)c1 * 65536 + (int64_t)c3 * (int64_t)dT / 128;
  int32_t pressure = (raw * sens / 2097152 - offset )/ 32768;
  return pressure / 100.0;
}

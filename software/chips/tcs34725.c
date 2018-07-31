#include <math.h>

#include "nrf_drv_gpiote.h"
#include "nrf_log.h"

#include "permamote.h"
#include "tcs34725.h"

static const nrf_twi_mngr_t* twi_mngr_instance;
static tcs34725_config_t config = {
  .int_time = TCS34725_INTEGRATIONTIME_2_4MS,
  .gain     = TCS34725_GAIN_1X
};

void tcs34725_init(const nrf_twi_mngr_t* instance) {
  twi_mngr_instance = instance;
}

void tcs34725_config(void) {

}

void tcs34725_enable(void) {

}

void  tcs34725_disable(void){

}

void  tcs34725_read_channels(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* c) {

}

uint16_t tcs34725_calculate_cct(uint16_t r, uint16_t g, uint16_t b) {
  return 0;
}
uint16_t tcs34725_calculate_lux(uint16_t r, uint16_t g, uint16_t b) {
  return 0;
}

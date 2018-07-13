#include <math.h>

#include "nrf_drv_gpiote.h"

#include "max44009.h"
#include "permamote.h"

static const nrf_twi_mngr_t* twi_mngr_instance;
static uint8_t int_status_buf[2] = {MAX44009_INT_STATUS, 0};
static uint8_t int_enable_buf[2] = {MAX44009_INT_EN, 0};
static uint8_t lux_read_buf[2] = {0};
static uint8_t config_buf[2] = {MAX44009_CONFIG, 0};
static uint8_t thresh_buf[2] = {MAX44009_THRESH_HI, 0};
static uint8_t time_buf[2] = {MAX44009_INT_TIME, 6};
static const uint8_t max44009_lux_high_addr = MAX44009_LUX_HI;
static const uint8_t max44009_lux_low_addr = MAX44009_LUX_LO;

static max44009_read_lux_callback* lux_read_callback;
static max44009_interrupt_callback* interrupt_callback;

static void lux_callback(ret_code_t result, void* p_context);

  static nrf_twi_mngr_transfer_t const int_status_transfer[] = {
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, int_status_buf, 1, NRF_TWI_MNGR_NO_STOP),
  NRF_TWI_MNGR_READ(MAX44009_ADDR, int_status_buf+1, 1, 0)
};

static nrf_twi_mngr_transfer_t const int_enable_transfer[] = {
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, int_enable_buf, 2, NRF_TWI_MNGR_NO_STOP),
};

static nrf_twi_mngr_transfer_t const lux_read_transfer[] = {
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, &max44009_lux_high_addr, 1, NRF_TWI_MNGR_NO_STOP),
  NRF_TWI_MNGR_READ(MAX44009_ADDR, lux_read_buf, 1, 0),
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, &max44009_lux_low_addr, 1, NRF_TWI_MNGR_NO_STOP),
  NRF_TWI_MNGR_READ(MAX44009_ADDR, lux_read_buf+1, 1, 0)
};

static nrf_twi_mngr_transfer_t const config_write_transfer[] = {
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, config_buf, 2, 0)
};

static nrf_twi_mngr_transfer_t const threshold_write_transfer[] = {
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, thresh_buf, 2, 0),
};

static nrf_twi_mngr_transfer_t const int_time_write_transfer[] = {
  NRF_TWI_MNGR_WRITE(MAX44009_ADDR, time_buf, 2, 0),
};

static nrf_twi_mngr_transaction_t const lux_read_transaction =
{
  .callback = lux_callback,
  .p_user_data = NULL,
  .p_transfers = lux_read_transfer,
  .number_of_transfers = sizeof(lux_read_transfer)/sizeof(lux_read_transfer[0]),
  .p_required_twi_cfg = NULL
};

static void interrupt_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, int_status_transfer, sizeof(int_status_transfer)/sizeof(int_status_transfer[0]), NULL);
  APP_ERROR_CHECK(error);

  if(int_status_buf[1] == 1) {
    interrupt_callback();
  }
}

static float calc_lux(void) {
  uint8_t exp = (lux_read_buf[0] & 0xF0) >> 4;
  uint8_t mant = (lux_read_buf[0] & 0x0F) << 4;
  mant |= lux_read_buf[1] & 0xF;
  return (float)(1 << exp) * (float)mant * 0.045;
}

static void lux_callback(ret_code_t result, void* p_context) {
  lux_read_callback(calc_lux());
}

void max44009_init(const nrf_twi_mngr_t* instance) {
  twi_mngr_instance = instance;
}

void max44009_set_interrupt_callback(max44009_interrupt_callback* callback) {
  interrupt_callback = callback;

  // setup gpiote interrupt
  if (!nrf_drv_gpiote_is_init()) {
    nrf_drv_gpiote_init();
  }
  nrf_drv_gpiote_in_config_t int_gpio_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(0);
  int_gpio_config.pull = NRF_GPIO_PIN_NOPULL;
  nrf_drv_gpiote_in_init(MAX44009_INT, &int_gpio_config, interrupt_handler);
}

void max44009_enable_interrupt(void) {
  int_enable_buf[1] = 1;
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, int_enable_transfer, sizeof(int_enable_transfer)/sizeof(int_enable_transfer[0]), NULL);
  APP_ERROR_CHECK(error);
  error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, int_time_write_transfer, sizeof(int_time_write_transfer)/sizeof(int_time_write_transfer[0]), NULL);
  APP_ERROR_CHECK(error);

  nrf_drv_gpiote_in_event_enable(MAX44009_INT, 1);
}

void max44009_disable_interrupt(void) {
  int_enable_buf[1] = 0;
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, int_enable_transfer, sizeof(int_enable_transfer)/sizeof(int_enable_transfer[0]), NULL);
  APP_ERROR_CHECK(error);

  nrf_drv_gpiote_in_event_enable(MAX44009_INT, 0);
}

void max44009_config(max44009_config_t config) {
  uint8_t config_byte = config.continuous << 7 |
                        config.manual << 6 |
                        config.cdr << 3 |
                        (config.int_time & 0x7);

  config_buf[1] = config_byte;

  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, config_write_transfer, sizeof(config_write_transfer)/sizeof(config_write_transfer[0]), NULL);
  APP_ERROR_CHECK(error);
}

void  max44009_set_read_lux_callback(max44009_read_lux_callback* callback) {
  lux_read_callback = callback;
}

//void calc_mant_exp (float lux, bool upper, uint8_t* exp, uint8_t* mant) {
//  static uint16_t max_mantissa = 128+64+32+16+15;
//  *mant = 15;
//  //if(upper) {
//  //  *mant += 15;
//  //  max_mantissa += 15;
//  //}
//  //if(lux < 11.5) {
//  //  *exp = 0;
//  //} else {
//    float remainder = lux / max_mantissa / 0.045;
//    *exp = ceil(log2(remainder));
//    //if (*exp > 0xE) {
//    //  *exp = 0xE;
//    //}
//  //}
//  *mant = (uint8_t)(lux / 0.045) >> *exp;
//  printf("\n%d calc_lux: %f\n", upper, *mant*0.45*(1<<*exp));
//  printf("exp: %x, mant: %x\n", *exp, *mant);
//  //if(lux >= 11.5) {
//  //  APP_ERROR_CHECK_BOOL(*mant & 0x8);
//  //}
//}

void max44009_set_upper_threshold(float thresh) {
  uint8_t exp, mant = 0;
  //calc_mant_exp(upper_threshold, true, &exp, &mant);
  static const unsigned int max_mantissa = 128+64+32+16+15;
  float remainder = (thresh/ max_mantissa / 0.045);
  printf("upper\n");
  printf("remainder: %f\n", remainder);
  exp = ceil(log2(remainder));
  mant = 15 + ((unsigned int)(thresh / 0.045) >> exp);
  float calc_lux = 0.045*(mant & 0xF0)*(1 << exp);
  printf("thresh: %f, exp: %x, mant: %x, calc lux: %f\n", thresh, exp, (mant & 0xF0), calc_lux);

  thresh_buf[0] = MAX44009_THRESH_HI;
  thresh_buf[1] = (exp << 4) | ((mant & 0xF0) >> 4);

  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, threshold_write_transfer, sizeof(threshold_write_transfer)/sizeof(threshold_write_transfer[0]), NULL);
  APP_ERROR_CHECK(error);

}

void max44009_set_lower_threshold(float thresh) {
  uint8_t exp, mant;
  //calc_mant_exp(lower_threshold, false, &exp, &mant);
  static const unsigned int max_mantissa = 128+64+32+16;
  float remainder = (thresh/ max_mantissa / 0.045);
  printf("lower\n");
  printf("remainder: %f\n", remainder);
  exp = ceil(log2(remainder));
  mant = (unsigned int)(thresh/ 0.045) >> exp;
  float calc_lux = 0.045*(mant & 0xF0)*(1 << exp);
  printf("thresh: %f, exp: %x, mant: %x, calc lux: %f\n", thresh, exp, (mant & 0xF0), calc_lux);


  thresh_buf[0] = MAX44009_THRESH_LO;
  thresh_buf[1] = (exp << 4) | ((mant & 0xF0) >> 4);

  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, threshold_write_transfer, sizeof(threshold_write_transfer)/sizeof(threshold_write_transfer[0]), NULL);
  APP_ERROR_CHECK(error);
}

void max44009_schedule_read_lux(void) {
  APP_ERROR_CHECK(nrf_twi_mngr_schedule(twi_mngr_instance, &lux_read_transaction));
}

float max44009_read_lux(void) {
  nrf_twi_mngr_perform(twi_mngr_instance, NULL, lux_read_transfer, sizeof(lux_read_transfer)/sizeof(lux_read_transfer[0]), NULL);
  return calc_lux();
}

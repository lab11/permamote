#include <stdlib.h>
#include <math.h>

#include "nrf_error.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "nrfx_timer.h"
#include "nrf_clock.h"
#include "nrfx_ppi.h"
#include "nrfx_spi.h"
#include "nrfx_spis.h"
#include "app_timer.h"
#include "app_scheduler.h"

#include "hm01b0.h"

#include "custom_board.h"

APP_TIMER_DEF(ae_watchdog);

static uint8_t* current_image;
static size_t   current_image_size;
static uint32_t current_size;
static uint32_t fvld_count;
static bool     watchdog_triggered = false;

static bool _initialized = false;
static const nrfx_timer_t mclk_timer = NRFX_TIMER_INSTANCE(3);
static nrf_ppi_channel_t mclk_ppi_channel;

static const nrfx_spis_t camera_spis_instance = NRFX_SPIS_INSTANCE(2);

static const nrf_twi_mngr_t* twi_mngr_instance;

int32_t hm01b0_write_reg(uint16_t reg, const uint8_t* buf, size_t len);
int32_t hm01b0_read_reg(uint16_t reg, uint8_t* buf, size_t len);
static int32_t hm01b0_load_script(const hm_script_t* psScript, uint32_t ui32ScriptCmdNum);

static void fvld_interrupt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    fvld_count ++;
}

void auto_exposure_watchdog_handler(void* m){
    watchdog_triggered = true;
}

//*****************************************************************************
//
//! @brief Write HM01B0 registers
//!
//! @param reg                  - Register address.
//! @param buf                  - Pointer to the data to be written.
//! @param len                  - Length of the data in bytes to be written.
//!
//! This function writes value to HM01B0 registers.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_write_reg(uint16_t reg, const uint8_t* buf, size_t len) {

  if (len > 16) return NRF_ERROR_INVALID_LENGTH;

  //
  // Create the transaction.
  //
  uint8_t regb[18];
  regb[0] = 0xFF & (reg >> 8);
  regb[1] = 0xFF & reg;
  memcpy(regb+2, buf, len);

  nrf_twi_mngr_transfer_t const write_transfer[] = {
    NRF_TWI_MNGR_WRITE(HM01B0_DEFAULT_ADDRESS, regb, len+2, 0),
  };

  //
  // Execute the transction.
  //
  return nrf_twi_mngr_perform(twi_mngr_instance, NULL, write_transfer, 1, NULL);
}

//*****************************************************************************
//
//! @brief Read HM01B0 registers
//!
//! @param reg                  - Register address.
//! @param buf                  - Pointer to the buffer for read data to be put
//! into.
//! @param len                  - Length of the data to be read.
//!
//! This function reads value from HM01B0 registers.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_read_reg(uint16_t reg, uint8_t* buf, size_t len) {
  //
  // Create the transaction.
  //

  uint8_t regb[2];
  regb[0] = 0xFF & (reg >> 8);
  regb[1] = 0xFF & reg;

  nrf_twi_mngr_transfer_t const read_transfer[] = {
    NRF_TWI_MNGR_WRITE(HM01B0_DEFAULT_ADDRESS, regb, sizeof(regb), 0),
    NRF_TWI_MNGR_READ(HM01B0_DEFAULT_ADDRESS, buf, len, 0),
  };

  //
  // Execute the transction.
  //
  return nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_transfer, 2, NULL);
}

//*****************************************************************************
//
//! @brief Load HM01B0 a given script
//!
//! @param script               - Pointer to the script to be loaded.
//! @param cmd_num              - Number of entries in a given script.
//!
//! This function loads HM01B0 a given script.
//!
//! @return err_code code.
//
//*****************************************************************************
static int32_t hm01b0_load_script(const hm_script_t* script, uint32_t cmd_num) {
  int err_code = 0;

  for (size_t idx = 0; idx < cmd_num; idx++) {
    err_code = hm01b0_write_reg((script + idx)->ui16Reg,
                            &((script + idx)->ui8Val),
                            sizeof(uint8_t));
    if (err_code != NRF_SUCCESS) {
      break;
    }
  }

  return err_code;
}

//*****************************************************************************
//
//! @brief Power up HM01B0
//!
//! This function powers up HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_power_up(void) {
  hm01b0_init_if();
  // Start mclk for camera
  hm01b0_mclk_enable();
  // Turn on power gate
  nrf_gpio_pin_clear(HM01B0_ENn);
  // Delay max turn-on time for camera
  nrf_delay_us(60);
}

//*****************************************************************************
//
//! @brief Power down HM01B0
//!
//! This function powers down HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_power_down(void) {
  // Disable mclk
  hm01b0_mclk_disable();
  // Disable power gate to camera
  nrf_gpio_pin_set(HM01B0_ENn);
  // Clear digital pins to camera
  nrf_gpio_cfg_output(HM01B0_PCLKO);
  nrf_gpio_cfg_output(HM01B0_LVLD);
  nrf_gpio_cfg_output(HM01B0_CAM_D0);
  nrf_gpio_cfg_output(HM01B0_CAM_TRIG);
  nrf_gpio_cfg_output(HM01B0_CAM_INT);
  nrf_gpio_pin_clear(HM01B0_LVLD);
  nrf_gpio_pin_clear(HM01B0_CAM_D0);
  nrf_gpio_pin_clear(HM01B0_CAM_TRIG);
  nrf_gpio_pin_clear(HM01B0_CAM_INT);

  hm01b0_deinit_if();
}

// Empty timer handler for MCLK timer generator
static void timer_handler(nrf_timer_event_t event_type, void *context) {

}

// Helper function to shift an array 2 bits to the right
static void shift_right_by_2(uint8_t* arr, size_t len) {
  for(size_t i = len-1; i > 0; --i) {
    arr[i] = (arr[i] >> 2 | (arr[i-1] & 0x3) << 6);
  }
  arr[0] = 0;
}

// Helper function to correct an image captured over SPI
static void correct_image(uint8_t* arr) {
  // clear border pixels
  for(size_t i = 0; i < HM01B0_RAW_IMAGE_SIZE; i+=HM01B0_PIXEL_X_NUM) {
    shift_right_by_2(arr+i, HM01B0_PIXEL_X_NUM);
    arr[i] = 0;
    arr[i + 1] = 0;
    arr[i + HM01B0_PIXEL_X_NUM - 2] = 0;
    arr[i + HM01B0_PIXEL_X_NUM - 1] = 0;
  }
}

// Helper function to shear off 2-pixel border
// TODO fix magic numbers
void reshape_to_320(uint8_t* arr) {
  size_t j = 2 + HM01B0_PIXEL_X_NUM*2;
  for(size_t i = 0; i < HM01B0_FULL_FRAME_PIXEL_Y_NUM; i++) {
    memcpy(arr + i*HM01B0_FULL_FRAME_PIXEL_X_NUM, arr + j, HM01B0_FULL_FRAME_PIXEL_X_NUM);
    j = j + HM01B0_PIXEL_X_NUM;
  }
}

// helper to address flattened buffer
uint8_t* idex(uint8_t* arr, size_t x, size_t y) {
  return arr + x + y * HM01B0_FULL_FRAME_PIXEL_X_NUM;
}

// downsample to 160x160 image
void downsample_160(uint8_t* arr) {
  for (size_t x = 0; x < HM01B0_FULL_FRAME_PIXEL_X_NUM; x += 4) {
    for (size_t y = 0; y < HM01B0_FULL_FRAME_PIXEL_Y_NUM; y += 4) {
      size_t i = x / 2;
      size_t j = y / 2;
      uint8_t blue =    ((uint32_t) *idex(arr, x+0, y+0) + *idex(arr, x+2, y+0) + *idex(arr, x+0, y+2) + *idex(arr, x+2, y+2)) / 4;
      uint8_t greenr =  ((uint32_t) *idex(arr, x+1, y+0) + *idex(arr, x+0, y+3) + *idex(arr, x+2, y+1) + *idex(arr, x+3, y+2)) / 4;
      uint8_t greenl =  ((uint32_t) *idex(arr, x+0, y+1) + *idex(arr, x+1, y+2) + *idex(arr, x+3, y+0) + *idex(arr, x+2, y+3)) / 4;
      uint8_t red =     ((uint32_t) *idex(arr, x+1, y+1) + *idex(arr, x+3, y+1) + *idex(arr, x+1, y+3) + *idex(arr, x+3, y+3)) / 4;
      *idex(arr, i, j)     = blue;
      *idex(arr, i, j+1)   = greenr;
      *idex(arr, i+1, j)   = greenl;
      *idex(arr, i+1, j+1) = red;
    }
  }
  for (size_t i = 0; i < 160; i ++) {
    memcpy(arr + i * 160, arr + i * 320, 160);
  }

}

// SPIS handler to auto-increment on every line read in
static void camera_spis_handler(nrfx_spis_evt_t  const * event, void *context) {
  //TODO actually get correct event type
  switch(event->evt_type) {
    case NRFX_SPIS_XFER_DONE:
      current_size += HM01B0_PIXEL_X_NUM;
      if (current_size < current_image_size) {
        APP_ERROR_CHECK(nrfx_spis_buffers_set(&camera_spis_instance, NULL, 0, current_image + current_size, HM01B0_PIXEL_X_NUM));
      }
      break;
    case NRFX_SPIS_BUFFERS_SET_DONE:
      //NRF_LOG_INFO("SPIS ready!");
      break;
    default:
      break;
  }
}

//*****************************************************************************
//
//! @brief Initialize MCLK
//!
//! This function initializes a Timer, PPI, and GPIOTE to generate MCLK for HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_mclk_init() {
  //
  // Set up timer.
  //
  nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
  timer_config.frequency = NRF_TIMER_FREQ_16MHz;
  int err_code = nrfx_timer_init(&mclk_timer, &timer_config, timer_handler);
  APP_ERROR_CHECK(err_code);
  nrfx_timer_extended_compare(&mclk_timer, NRF_TIMER_CC_CHANNEL0, 1, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

  //
  // Configure timer output pin.
  //
  if (!nrfx_gpiote_is_init()) {
    nrfx_gpiote_init();
  }
  nrfx_gpiote_out_config_t gpio_config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(0);
  nrfx_gpiote_out_init(HM01B0_MCLK, &gpio_config);

  //
  // Set up PPI interface
  //
  uint32_t mclk_gpio_task_addr = nrfx_gpiote_out_task_addr_get(HM01B0_MCLK);
  uint32_t timer_compare_event_addr = nrfx_timer_compare_event_address_get(&mclk_timer, NRF_TIMER_CC_CHANNEL0);

  // setup ppi channel so that timer compare event is triggering GPIO toggle
  err_code = nrfx_ppi_channel_alloc(&mclk_ppi_channel);
  APP_ERROR_CHECK(err_code);

  err_code = nrfx_ppi_channel_assign(mclk_ppi_channel, timer_compare_event_addr, mclk_gpio_task_addr);
  APP_ERROR_CHECK(err_code);

}

//*****************************************************************************
//
//! @brief Enable MCLK
//!
//! This function enables the timer and PPI channel for MCLK
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_mclk_enable() {
  //
  // Start the timer, enable PPI
  //
  nrfx_gpiote_out_task_enable(HM01B0_MCLK);
  nrfx_timer_enable(&mclk_timer);
  int err_code = nrfx_ppi_channel_enable(mclk_ppi_channel);
  APP_ERROR_CHECK(err_code);
}

//*****************************************************************************
//
//! @brief Disable MCLK
//!
//! This function disable CTimer to stop MCLK for HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_mclk_disable() {
  //
  // Stop the timer
  //
  nrf_ppi_channel_disable(mclk_ppi_channel);
  nrfx_gpiote_out_task_disable(HM01B0_MCLK);
  nrfx_timer_disable(&mclk_timer);
}

void hm01b0_init_i2c(const nrf_twi_mngr_t* instance) {
  //
  // Initialize I2C
  //
  twi_mngr_instance = instance;

  // create watchdog timer
  app_timer_create(&ae_watchdog, APP_TIMER_MODE_SINGLE_SHOT, auto_exposure_watchdog_handler);
}

//*****************************************************************************
//
//! @brief Initialize interfaces
//!
//! This function initializes interfaces.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_init_if(void) {
  int32_t error = NRF_SUCCESS;

  // initialize SPI for camera interface.
  // Enable the constant latency sub power mode to minimize the time it takes
  // for the SPIS peripheral to become active after the CSN line is asserted
  // (when the CPU is in sleep mode).
  NRF_POWER->TASKS_CONSTLAT = 1;
  nrfx_spis_config_t camera_spis_config = NRFX_SPIS_DEFAULT_CONFIG;
  camera_spis_config.mode = NRF_SPIS_MODE_1;
  camera_spis_config.sck_pin = HM01B0_PCLKO;
  camera_spis_config.mosi_pin = HM01B0_CAM_D0;
  camera_spis_config.csn_pin = HM01B0_LVLD;
  error = nrfx_spis_init(&camera_spis_instance, &camera_spis_config, camera_spis_handler, NULL);
  if (error) return error;

  // set up interrupt for FVLD
  if (!nrfx_gpiote_is_init()) {
    nrfx_gpiote_init();
  }
  nrfx_gpiote_in_config_t int_gpio_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(1);
  int_gpio_config.pull = NRF_GPIO_PIN_NOPULL;
  error = nrfx_gpiote_in_init(HM01B0_FVLD, &int_gpio_config, fvld_interrupt_handler);
  if (error) return error;

  _initialized = true;

  return NRF_SUCCESS;
}

//*****************************************************************************
//
//! @brief Deinitialize interfaces
//!
//! This function deinitializes interfaces.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_deinit_if(void) {
  //
  // Deinit SPI.
  //
  NRF_POWER->TASKS_LOWPWR = 1;
  NRF_POWER->TASKS_CONSTLAT = 0;
  if (_initialized) {
    nrfx_spis_uninit(&camera_spis_instance);
    nrfx_gpiote_in_uninit(HM01B0_FVLD);
  }
  _initialized = false;
  return 0;
}

//*****************************************************************************
//
//! @brief Get HM01B0 Gain Settings
//!
//! @param again - Pointer to buffer for the read back analog gain.
//! @param dgain - Pointer to buffer for the read back digital gain.
//!
//! This function reads back HM01B0 gain settings.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_get_gain(uint8_t *again, uint16_t *dgain) {
  uint8_t data[1];
  int err_code;

  *again = 0;
  *dgain = 0;

  err_code =
      hm01b0_read_reg(HM01B0_REG_ANALOG_GAIN, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *again |= (data[0] >> 4) & 0x3;
  } else return err_code;

  err_code =
      hm01b0_read_reg(HM01B0_REG_DIGITAL_GAIN_H, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *dgain |= (data[0] & 0x3) << 8;
  } else return err_code;

  err_code =
      hm01b0_read_reg(HM01B0_REG_DIGITAL_GAIN_L, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *dgain |= data[0];
  }

  return err_code;
}

//*****************************************************************************
//
//! @brief Get HM01B0 Integration Setting
//!
//! @param integration - Pointer to buffer for the read back integration setting.
//!
//! This function reads back HM01B0 integration settings.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_get_integration(uint16_t *integration) {
  uint8_t data[1];
  int err_code;

  *integration = 0;

  err_code =
      hm01b0_read_reg(HM01B0_REG_INTEGRATION_H, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *integration |= data[0] << 8;
  } else return err_code;

  err_code =
      hm01b0_read_reg(HM01B0_REG_INTEGRATION_L, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *integration |= (data[0]);
  }

  return err_code;
}

//*****************************************************************************
//
//! @brief Set HM01B0 Integration Setting
//!
//! @param integration - integration setting.
//!
//! This function writes HM01B0 integration settings.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_set_integration(uint16_t integration) {
  int err_code;
  uint8_t data[2];

  data[0] = (integration >> 8) & 0xFF;
  data[1] = integration & 0xFF;

  err_code =
    hm01b0_write_reg(HM01B0_REG_INTEGRATION_H, data, 1);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  err_code =
    hm01b0_write_reg(HM01B0_REG_INTEGRATION_L, data + 1, 1);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  uint8_t consume = HM01B0_REG_GRP_PARAM_HOLD_CONSUME;
  err_code =
    hm01b0_write_reg(HM01B0_REG_GRP_PARAM_HOLD, &consume, 1);

  return err_code;
}

//*****************************************************************************
//
//! @brief Set HM01B0 Gain Settings
//!
//! @param again -  analog gain.
//! @param dgain -  digital gain.
//!
//! This function sets HM01B0 gain settings.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_set_gain(uint8_t again, uint16_t dgain) {
  int err_code;
  uint8_t data[2];

  data[0] = again << 4;

  err_code =
    hm01b0_write_reg(HM01B0_REG_ANALOG_GAIN, data, 1);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  data[0] = (dgain >> 8) & 0xFF;
  data[1] = dgain & 0xFF;

  err_code =
    hm01b0_write_reg(HM01B0_REG_DIGITAL_GAIN_H, data, 1);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  err_code =
    hm01b0_write_reg(HM01B0_REG_DIGITAL_GAIN_L, data + 1, 1);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  uint8_t consume = HM01B0_REG_GRP_PARAM_HOLD_CONSUME;
  err_code =
    hm01b0_write_reg(HM01B0_REG_GRP_PARAM_HOLD, &consume, 1);

  return err_code;
}

//*****************************************************************************
//
//! @brief Enable HM01B0 Auto Exposure
//!
//! This function enables HM01B0 auto exposure.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_enable_ae(void) {
  int err_code;
  uint8_t enable = 1;

  err_code =
    hm01b0_write_reg(HM01B0_REG_AE_CTRL, &enable, 1);

  return err_code;
}

//*****************************************************************************
//
//! @brief Disable HM01B0 Auto Exposure
//!
//! This function disables HM01B0 auto exposure.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_disable_ae(void) {
  int err_code;
  uint8_t disable = 0;

  err_code =
    hm01b0_write_reg(HM01B0_REG_AE_CTRL, &disable, 1);

  return err_code;
}

//*****************************************************************************
//
//! @brief Get HM01B0 frame PCK length
//!
//! @param pck_length - Pointer to buffer for the read back PCK length setting.
//!
//! This function reads back HM01B0 PCK length setting.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_get_line_pck_length(uint16_t *pck_length) {
  uint8_t data[1];
  int err_code;

  *pck_length = 0;

  err_code =
      hm01b0_read_reg(HM01B0_REG_LINE_LENGTH_PCK_H, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *pck_length |= data[0] << 8;
  } else return err_code;

  err_code =
      hm01b0_read_reg(HM01B0_REG_LINE_LENGTH_PCK_L, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *pck_length |= (data[0]);
  }

  return err_code;
}

//*****************************************************************************
//
//! @brief Get HM01B0 frame lines length
//!
//! @param lines_length - Pointer to buffer for the read back lines length setting.
//!
//! This function reads back HM01B0 lines length setting.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_get_frame_lines_length(uint16_t *lines_length) {
  uint8_t data[1];
  int err_code;

  *lines_length= 0;

  err_code =
      hm01b0_read_reg(HM01B0_REG_FRAME_LENGTH_LINES_H, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *lines_length |= data[0] << 8;
  } else return err_code;

  err_code =
      hm01b0_read_reg(HM01B0_REG_FRAME_LENGTH_LINES_L, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *lines_length |= (data[0]);
  }

  return err_code;
}

//*****************************************************************************
//
//! @brief Get HM01B0 exposure time
//!
//! @param integration - integration setting
//! @param pck_length - pck length setting
//!
//! This function calculates exposure time from integration time and pck_length
//!
//! @return calculated exposure time in seconds.
//
//*****************************************************************************
float hm01b0_get_exposure_time(uint16_t integration, uint16_t pck_length) {
  return (float) integration * (float) pck_length / ((float) HM01B0_MCLK_FREQ / 8);
}

//*****************************************************************************
//
//! @brief Get HM01B0 integration value from exposure time
//!
//! @param exposure_time - desired exposure time in seconds
//! @param pck_length - current pck length setting
//!
//! This function calculates integration value from desired exposure_time
//!
//! @return integration value
//
//*****************************************************************************
uint16_t hm01b0_exposure_to_integration(float exposure_time, uint16_t pck_length) {
  uint16_t integration = 2;

  if (pck_length == 0) pck_length = 1;

  float integration_f = exposure_time * ((float)HM01B0_MCLK_FREQ / 8) / pck_length;
  // cap at uint16_t maximum
  if (integration_f > 65535) {
    integration_f = 65535;
  }
  // Minimum is 2 lines integration time
  if (integration_f > 2) {
    integration = (uint16_t) roundf(integration_f);
  }

  return integration;
}


//*****************************************************************************
//
//! @brief Get HM01B0 Model ID
//!
//! @param model_id           - Pointer to buffer for the read back model ID.
//!
//! This function reads back HM01B0 model ID.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_get_modelid(uint16_t* model_id) {
  uint8_t data[1];
  int err_code;

  *model_id = 0x0000;

  err_code =
      hm01b0_read_reg(HM01B0_REG_MODEL_ID_H, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *model_id |= (data[0] << 8);
  }

  err_code =
      hm01b0_read_reg(HM01B0_REG_MODEL_ID_L, data, sizeof(data));
  if (err_code == NRF_SUCCESS) {
    *model_id |= data[0];
  }

  return err_code;
}

//*****************************************************************************
//
//! @brief Initialize HM01B0
//!
//! @param script               - Pointer to HM01B0 initialization script.
//! @param cmd_num              - No. of commands in HM01B0 initialization script.
//!
//! This function initilizes HM01B0 with a given script.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_init_system(const hm_script_t* script,
                            uint32_t cmd_num) {
  return hm01b0_load_script(script, cmd_num);
}

//*****************************************************************************
//
//! @brief Set HM01B0 in the walking 1s test mode
//!
//! This function sets HM01B0 in the walking 1s test mode.
//!
//! @return err_code code.
//
//*****************************************************************************
//uint32_t hm01b0_test_walking1s(hm01b0_cfg_t* psCfg) {
//  uint32_t ui32ScriptCmdNum =
//      sizeof(sHM01b0TestModeScript_Walking1s) / sizeof(hm_script_t);
//  hm_script_t* psScript = (hm_script_t*)sHM01b0TestModeScript_Walking1s;
//
//  return hm01b0_load_script(psCfg, psScript, ui32ScriptCmdNum);
//}

//*****************************************************************************
//
//! @brief Check the data read from HM01B0 in the walking 1s test mode
//!
//! @param pui8Buffer       - Pointer to data buffer.
//! @param ui32BufferLen    - Buffer length
//! @param ui32PrintCnt     - Number of mismatched data to be printed out
//!
//! This function sets HM01B0 in the walking 1s test mode.
//!
//! @return err_code code.
//
//*****************************************************************************
//void hm01b0_test_walking1s_check_data_sanity(uint8_t* pui8Buffer,
//                                             uint32_t ui32BufferLen,
//                                             uint32_t ui32PrintCnt) {
//  uint8_t ui8ByteData = *pui8Buffer;
//  uint32_t ui32MismatchCnt = 0x00;
//
//  for (uint32_t ui32Idx = 0; ui32Idx < ui32BufferLen; ui32Idx++) {
//    if (*(pui8Buffer + ui32Idx) != ui8ByteData) {
//      if (ui32PrintCnt) {
//        am_util_stdio_printf("[0x%08X] actual 0x%02X expected 0x%02X\n",
//                             ui32Idx, *(pui8Buffer + ui32Idx), ui8ByteData);
//        am_util_delay_ms(1);
//        ui32PrintCnt--;
//      }
//      ui32MismatchCnt++;
//    }
//
//    if (ui8ByteData)
//      ui8ByteData = ui8ByteData << 1;
//    else
//      ui8ByteData = 0x01;
//  }
//
//  am_util_stdio_printf("Mismatch Rate %d/%d\n", ui32MismatchCnt, ui32BufferLen);
//}

//*****************************************************************************
//
//! @brief Software reset HM01B0
//!
//! This function resets HM01B0 by issuing a reset command.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_reset_sw(void) {
  uint8_t buf[1] = {0x00};
  return hm01b0_write_reg(HM01B0_REG_SW_RESET, buf, sizeof(buf));
}

//*****************************************************************************
//
//! @brief Get current HM01B0 operation mode.
//!
//! @param mode         - Pointer to buffer
//!                     - for the read back operation mode to be put into
//!
//! This function get HM01B0 operation mode.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_get_mode(uint8_t* mode) {
  uint8_t data[1] = {0x01};
  int err_code;

  err_code =
      hm01b0_read_reg(HM01B0_REG_MODE_SELECT, data, sizeof(data));

  *mode = data[0];

  return err_code;
}

//*****************************************************************************
//
//! @brief Set HM01B0 operation mode.
//!
//! @param mode         - Operation mode. One of:
//!     HM01B0_REG_MODE_SELECT_STANDBY
//!     HM01B0_REG_MODE_SELECT_STREAMING
//!     HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES
//!     HM01B0_REG_MODE_SELECT_STREAMING_HW_TRIGGER
//! @param frame_cnt    - Frame count for
//! HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES.
//!                     - Discarded if other modes.
//!
//! This function set HM01B0 operation mode.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_set_mode(hm01b0_mode mode,
                         uint8_t frame_cnt) {
  int32_t err_code = NRF_SUCCESS;

  if (mode == STREAM_N) {
    err_code = hm01b0_write_reg(HM01B0_REG_PMU_PROGRAMMABLE_FRAMECNT,
                               &frame_cnt, sizeof(frame_cnt));
  }

  if (err_code == NRF_SUCCESS) {
    err_code = hm01b0_write_reg(HM01B0_REG_MODE_SELECT, &mode,
                               sizeof(mode));
  }

  return err_code;
}

//*****************************************************************************
//
//! @brief Hardware trigger HM01B0 to stream.
//!
//! @param psCfg        - Pointer to HM01B0 configuration structure.
//! @param bTrigger     - True to start streaming
//!                     - False to stop streaming
//!
//! This function triggers HM01B0 to stream by toggling the TRIG pin.
//!
//! @return err_code code.
//
//*****************************************************************************
//uint32_t hm01b0_hardware_trigger_streaming(hm01b0_cfg_t* psCfg, bool bTrigger) {
//  uint32_t ui32Err = HM01B0_ERR_OK;
//  uint8_t ui8Mode;
//
//  ui32Err = hm01b0_get_mode(psCfg, &ui8Mode);
//
//  if (ui32Err != HM01B0_ERR_OK) goto end;
//
//  if (ui8Mode != HM01B0_REG_MODE_SELECT_STREAMING_HW_TRIGGER) {
//    ui32Err = HM01B0_ERR_MODE;
//    goto end;
//  }
//
//  if (bTrigger) {
//    am_hal_gpio_output_set(psCfg->ui8PinTrig);
//  } else {
//    am_hal_gpio_output_clear(psCfg->ui8PinTrig);
//  }
//
//end:
//  return ui32Err;
//}

//*****************************************************************************
//
//! @brief Set HM01B0 mirror mode.
//!
//! @param psCfg        - Pointer to HM01B0 configuration structure.
//! @param bHmirror     - Horizontal mirror
//! @param bVmirror     - Vertical mirror
//!
//! This function set HM01B0 mirror mode.
//!
//! @return err_code code.
//
//*****************************************************************************
//uint32_t hm01b0_set_mirror(hm01b0_cfg_t* psCfg, bool bHmirror, bool bVmirror) {
//  uint8_t ui8Data = 0x00;
//  uint32_t ui32Err = HM01B0_ERR_OK;
//
//  if (bHmirror) {
//    ui8Data |= HM01B0_REG_IMAGE_ORIENTATION_HMIRROR;
//  }
//
//  if (bVmirror) {
//    ui8Data |= HM01B0_REG_IMAGE_ORIENTATION_VMIRROR;
//  }
//
//  ui32Err = hm01b0_write_reg(psCfg, HM01B0_REG_IMAGE_ORIENTATION, &ui8Data,
//                             sizeof(ui8Data));
//
//  if (ui32Err == HM01B0_ERR_OK) {
//    ui8Data = HM01B0_REG_GRP_PARAM_HOLD_HOLD;
//    ui32Err = hm01b0_write_reg(psCfg, HM01B0_REG_GRP_PARAM_HOLD, &ui8Data,
//                               sizeof(ui8Data));
//  }
//
//  return ui32Err;
//}

int32_t hm01b0_wait_for_autoexposure(void) {
  int32_t error = 0;
  fvld_count = 0;

  error = app_timer_start(ae_watchdog, APP_TIMER_TICKS(3000), NULL);
  watchdog_triggered = false;
  APP_ERROR_CHECK(error);
  //if(error) return error;

  // enable event for FVLD interrupt
  nrfx_gpiote_in_event_enable(HM01B0_FVLD, 1);

  // set mode to streaming
  error = hm01b0_set_mode(STREAMING, 0);
  if(error) return error;

  // wait for 5 frames
  while(fvld_count < 4) {
    app_sched_execute();
    NRF_LOG_INFO("FVLD count: %d", fvld_count);
    if (watchdog_triggered) {
      error = NRF_ERROR_TIMEOUT;
      break;
    }
    __WFI();
  }

  nrfx_gpiote_in_event_disable(HM01B0_FVLD);

  error = app_timer_stop(ae_watchdog);

  if (error) {
    hm01b0_set_mode(STANDBY, 1);
    return error;
  } else {
    return hm01b0_set_mode(STANDBY, 1);
  }
}

//*****************************************************************************
//
//! @brief Read data of one frame from HM01B0.
//!
//! @param buf              - Pointer to the frame buffer.
//! @param len              - Framebuffer size.
//!
//! This function read data of one frame from HM01B0.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_blocking_read_oneframe(uint8_t *buf, size_t len) {
  int32_t err_code = NRF_SUCCESS;
  current_image = buf;
  current_image_size = len;
  current_size = 0;

  // set mode to streaming
  hm01b0_set_mode(STREAMING, 1);

  while(!nrf_gpio_pin_read(HM01B0_FVLD)) {

  }

  APP_ERROR_CHECK(nrfx_spis_buffers_set(&camera_spis_instance, NULL, 0, buf, HM01B0_PIXEL_X_NUM));
  while(current_size < len) {
    __WFI();
  }

  correct_image(buf);
  reshape_to_320(buf);
  current_image = NULL;

  hm01b0_set_mode(STANDBY, 0);

  return err_code;
}

//  uint32_t ui32Err = HM01B0_ERR_OK;
//  uint32_t ui32Idx = 0x00;
//
//  am_util_stdio_printf("[%s] +\n", __func__);
//#ifdef ENABLE_ASYNC
//  while (!s_bVsyncAsserted);
//
//  while (s_bVsyncAsserted) {
//    // we don't check HSYNC here on the basis of assuming HM01B0 in the gated
//    // PCLK mode which PCLK toggles only when HSYNC is asserted. And also to
//    // minimize the overhead of polling.
//
//    if (read_pclk()) {
//      *(pui8Buffer + ui32Idx++) = read_byte();
//
//      if (ui32Idx == ui32BufferLen) {
//        goto end;
//      }
//
//      while (read_pclk());
//    }
//  }
//#else
//  uint32_t ui32HsyncCnt = 0x00;
//
//  while ((ui32HsyncCnt < HM01B0_PIXEL_Y_NUM)) {
//    while (0x00 == read_hsync());
//
//    // read one row
//    while (read_hsync()) {
//      while (0x00 == read_pclk());
//
//      *(pui8Buffer + ui32Idx++) = read_byte();
//
//      if (ui32Idx == ui32BufferLen) {
//        goto end;
//      }
//
//      while (read_pclk());
//    }
//
//    ui32HsyncCnt++;
//  }
//#endif
//end:
//  am_util_stdio_printf("[%s] - Byte Counts %d\n", __func__, ui32Idx);
//  return ui32Err;
//}
//
//uint32_t hm01b0_single_frame_capture(hm01b0_cfg_t* psCfg) {
//  hm01b0_write_reg(psCfg, HM01B0_REG_PMU_PROGRAMMABLE_FRAMECNT, 0x01, 1);
//  hm01b0_write_reg(psCfg, HM01B0_REG_MODE_SELECT,
//                   HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES, 1);
//  hm01b0_write_reg(psCfg, HM01B0_REG_GRP_PARAM_HOLD, 0x01, 1);
//}

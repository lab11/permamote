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

#include "hm01b0.h"

#include "custom_board.h"

static uint8_t* current_image;
static size_t   current_image_size;
static uint32_t current_size;

static const nrfx_timer_t mclk_timer = NRFX_TIMER_INSTANCE(3);
static nrf_ppi_channel_t mclk_ppi_channel;

static const nrfx_spis_t camera_spis_instance = NRFX_SPIS_INSTANCE(2);

static const nrf_twi_mngr_t* twi_mngr_instance;

int32_t hm01b0_write_reg(uint16_t reg, const uint8_t* buf, size_t len);
int32_t hm01b0_read_reg(uint16_t reg, uint8_t* buf, size_t len);
static int32_t hm01b0_load_script(const hm_script_t* psScript, uint32_t ui32ScriptCmdNum);


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
  nrf_delay_us(50);
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
  // Clear all digital pins to camera
  //nrf_gpio_cfg_output(HM01B0_MCLK);
  nrf_gpio_cfg_output(HM01B0_PCLKO);
  nrf_gpio_cfg_output(HM01B0_FVLD);
  nrf_gpio_cfg_output(HM01B0_LVLD);
  nrf_gpio_cfg_output(HM01B0_CAM_D0);
  nrf_gpio_cfg_output(HM01B0_CAM_TRIG);
  nrf_gpio_cfg_output(HM01B0_CAM_INT);
  //nrf_gpio_pin_clear(HM01B0_MCLK);
  //nrf_gpio_pin_clear(HM01B0_PCLKO);
  nrf_gpio_pin_clear(HM01B0_FVLD);
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
  for(size_t i = 0; i < HM01B0_IMAGE_SIZE; i+=HM01B0_PIXEL_X_NUM) {
    shift_right_by_2(arr+i, HM01B0_PIXEL_X_NUM);
    arr[i] = 0;
    arr[i + 1] = 0;
    arr[i + HM01B0_PIXEL_X_NUM - 2] = 0;
    arr[i + HM01B0_PIXEL_X_NUM - 1] = 0;
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
  // initialize SPI for camera interface.
  // Enable the constant latency sub power mode to minimize the time it takes
  // for the SPIS peripheral to become active after the CSN line is asserted
  // (when the CPU is in sleep mode).
  nrfx_spis_config_t camera_spis_config = NRFX_SPIS_DEFAULT_CONFIG;
  camera_spis_config.mode = NRF_SPIS_MODE_1;
  camera_spis_config.sck_pin = HM01B0_PCLKO;
  camera_spis_config.mosi_pin = HM01B0_CAM_D0;
  camera_spis_config.csn_pin = HM01B0_LVLD;
  APP_ERROR_CHECK(nrfx_spis_init(&camera_spis_instance, &camera_spis_config, camera_spis_handler, NULL));

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
  nrfx_spis_uninit(&camera_spis_instance);

  return 0;
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

  // set mode to streaming, one frame
  hm01b0_set_mode(STREAM_N, 1);

  APP_ERROR_CHECK(nrfx_spis_buffers_set(&camera_spis_instance, NULL, 0, buf, HM01B0_PIXEL_X_NUM));
  while(current_size < len) {
    __WFI();
  }

  correct_image(buf);
  current_image = NULL;

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

#include "nrf_twi.h"
#include "nrf_delay.h"
#include "nrf_log.h"

#include "hm01b0.h"

#include "custom_board.h"

static const nrf_twi_mngr_t* twi_mngr_instance;

static int32_t hm01b0_write_reg(uint16_t reg, const uint8_t* buf, size_t len);
static int32_t hm01b0_read_reg(uint16_t reg, uint8_t* buf, size_t len);
static int32_t hm01b0_load_script(const hm_script_t* psScript, uint32_t ui32ScriptCmdNum);


//*****************************************************************************
//
//! @brief Write HM01B0 registers
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//! @param ui16Reg              - Register address.
//! @param pui8Value            - Pointer to the data to be written.
//! @param ui32NumBytes         - Length of the data in bytes to be written.
//!
//! This function writes value to HM01B0 registers.
//!
//! @return Error code.
//
//*****************************************************************************
static int32_t hm01b0_write_reg(uint16_t reg, const uint8_t* buf, size_t len) {
  //
  // Create the transaction.
  //
  nrf_twi_mngr_transfer_t const write_transfer[] = {
    NRF_TWI_MNGR_WRITE(HM01B0_DEFAULT_ADDRESS, (uint8_t*)&reg, sizeof(reg), NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_WRITE(HM01B0_DEFAULT_ADDRESS, buf, len, 0),
  };

  //
  // Execute the transction.
  //
  return nrf_twi_mngr_perform(twi_mngr_instance, NULL, write_transfer, 2, NULL);
}

//*****************************************************************************
//
//! @brief Read HM01B0 registers
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//! @param ui16Reg              - Register address.
//! @param pui8Value            - Pointer to the buffer for read data to be put
//! into.
//! @param ui32NumBytes         - Length of the data to be read.
//!
//! This function reads value from HM01B0 registers.
//!
//! @return Error code.
//
//*****************************************************************************
static int32_t hm01b0_read_reg(uint16_t reg, uint8_t* buf, size_t len) {
  //
  // Create the transaction.
  //

  uint8_t write_buffer[16];
  write_buffer[0] = 0xFF & (reg >> 1);
  write_buffer[1] = 0xFF & reg;
  //uint8_t size = len < sizeof(write_buffer) - sizeof(reg)? len : sizeof(write_buffer) - sizeof(reg);
  memcpy(write_buffer + sizeof(reg), buf, len);

  nrf_twi_mngr_transfer_t const read_transfer[] = {
    NRF_TWI_MNGR_WRITE(HM01B0_DEFAULT_ADDRESS, write_buffer, len+2, 0),
  };

  //
  // Execute the transction.
  //
  return nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_transfer, 1, NULL);
}

//*****************************************************************************
//
//! @brief Load HM01B0 a given script
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//! @param psScrip              - Pointer to the script to be loaded.
//! @param ui32ScriptCmdNum     - Number of entries in a given script.
//!
//! This function loads HM01B0 a given script.
//!
//! @return Error code.
//
//*****************************************************************************
static int32_t hm01b0_load_script(const hm_script_t* psScript, uint32_t ui32ScriptCmdNum) {
  int error = 0;

  for (size_t idx = 0; idx < ui32ScriptCmdNum; idx++) {
    error = hm01b0_write_reg((psScript + idx)->ui16Reg,
                            &((psScript + idx)->ui8Val),
                            sizeof(uint8_t));
    if (error != NRF_SUCCESS) {
      break;
    }
  }

  return error;
}

//*****************************************************************************
//
//! @brief Power up HM01B0
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//!
//! This function powers up HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_power_up(void) {
  nrf_gpio_pin_clear(HM01B0_ENn);
  nrf_delay_us(500);
}

//*****************************************************************************
//
//! @brief Power down HM01B0
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//!
//! This function powers up HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_power_down(void) {
  nrf_gpio_pin_set(HM01B0_ENn);
}

//*****************************************************************************
//
//! @brief Enable MCLK
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//!
//! This function utilizes CTimer to generate MCLK for HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_mclk_enable() {
  //
  // Set up timer.
  //

  //
  // Set the pattern in the CMPR registers.
  //

  //
  // Set the timer trigger and pattern length.
  //

  //
  // Configure timer output pin.
  //

  //
  // Start the timer.
  //
}

//*****************************************************************************
//
//! @brief Disable MCLK
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//!
//! This function disable CTimer to stop MCLK for HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_mclk_disable() {
  //
  // Stop the timer.
  //
}

//*****************************************************************************
//
//! @brief Initialize interfaces
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//!
//! This function initializes interfaces.
//!
//! @return Error code.
//
//*****************************************************************************
int32_t hm01b0_init_if(const nrf_twi_mngr_t* instance) {
  //
  // Initialize I2C
  //
  twi_mngr_instance = instance;

  // initialize pins for camera interface.


  return NRF_SUCCESS;
}

//*****************************************************************************
//
//! @brief Deinitialize interfaces
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//!
//! This function deinitializes interfaces.
//!
//! @return Error code.
//
//*****************************************************************************
int32_t hm01b0_deinit_if(void) {
  twi_mngr_instance = NULL;

  return 0;
}

//*****************************************************************************
//
//! @brief Get HM01B0 Model ID
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//! @param pui16MID             - Pointer to buffer for the read back model ID.
//!
//! This function reads back HM01B0 model ID.
//!
//! @return Error code.
//
//*****************************************************************************
int32_t hm01b0_get_modelid(uint16_t* model_id) {
  uint8_t data[1];
  int error;

  *model_id = 0x0000;

  error =
      hm01b0_read_reg(HM01B0_REG_MODEL_ID_H, data, sizeof(data));
  if (error == NRF_SUCCESS) {
    *model_id |= (data[0] << 8);
  }

  error =
      hm01b0_read_reg(HM01B0_REG_MODEL_ID_L, data, sizeof(data));
  if (error == NRF_SUCCESS) {
    *model_id |= data[0];
  }

  return error;
}

//*****************************************************************************
//
//! @brief Initialize HM01B0
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//! @param psScript             - Pointer to HM01B0 initialization script.
//! @param ui32ScriptCmdNum     - No. of commands in HM01B0 initialization
//! script.
//!
//! This function initilizes HM01B0 with a given script.
//!
//! @return Error code.
//
//*****************************************************************************
int32_t hm01b0_init_system(const hm_script_t* psScript,
                            uint32_t ui32ScriptCmdNum) {
  return hm01b0_load_script(psScript, ui32ScriptCmdNum);
}

//*****************************************************************************
//
//! @brief Set HM01B0 in the walking 1s test mode
//!
//! @param psCfg                - Pointer to HM01B0 configuration structure.
//!
//! This function sets HM01B0 in the walking 1s test mode.
//!
//! @return Error code.
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
//! @return Error code.
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
//! @param psCfg        - Pointer to HM01B0 configuration structure.
//!
//! This function resets HM01B0 by issuing a reset command.
//!
//! @return Error code.
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
//! @param psCfg        - Pointer to HM01B0 configuration structure.
//! @param pui8Mode     - Pointer to buffer
//!                     - for the read back operation mode to be put into
//!
//! This function get HM01B0 operation mode.
//!
//! @return Error code.
//
//*****************************************************************************
int32_t hm01b0_get_mode(uint8_t* mode) {
  uint8_t data[1] = {0x01};
  int error;

  error =
      hm01b0_read_reg(HM01B0_REG_MODE_SELECT, data, sizeof(data));

  *mode = data[0];

  return error;
}

//*****************************************************************************
//
//! @brief Set HM01B0 operation mode.
//!
//! @param psCfg        - Pointer to HM01B0 configuration structure.
//! @param ui8Mode      - Operation mode. One of:
//!     HM01B0_REG_MODE_SELECT_STANDBY
//!     HM01B0_REG_MODE_SELECT_STREAMING
//!     HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES
//!     HM01B0_REG_MODE_SELECT_STREAMING_HW_TRIGGER
//! @param ui8FrameCnt  - Frame count for
//! HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES.
//!                     - Discarded if other modes.
//!
//! This function set HM01B0 operation mode.
//!
//! @return Error code.
//
//*****************************************************************************
//uint32_t hm01b0_set_mode(hm01b0_cfg_t* psCfg, uint8_t ui8Mode,
//                         uint8_t ui8FrameCnt) {
//  uint32_t ui32Err = HM01B0_ERR_OK;
//
//  if (ui8Mode == HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES) {
//    ui32Err = hm01b0_write_reg(psCfg, HM01B0_REG_PMU_PROGRAMMABLE_FRAMECNT,
//                               &ui8FrameCnt, sizeof(ui8FrameCnt));
//  }
//
//  if (ui32Err == HM01B0_ERR_OK) {
//    ui32Err = hm01b0_write_reg(psCfg, HM01B0_REG_MODE_SELECT, &ui8Mode,
//                               sizeof(ui8Mode));
//  }
//
//  return ui32Err;
//}

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
//! @return Error code.
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
//! @return Error code.
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
//! @param psCfg            - Pointer to HM01B0 configuration structure.
//! @param pui8Buffer       - Pointer to the frame buffer.
//! @param ui32BufferLen    - Framebuffer size.
//!
//! This function read data of one frame from HM01B0.
//!
//! @return Error code.
//
//*****************************************************************************
//uint32_t hm01b0_blocking_read_oneframe(hm01b0_cfg_t* psCfg, uint8_t* pui8Buffer,
//                                       uint32_t ui32BufferLen) {
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

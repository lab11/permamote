#pragma once

#include "nrf_twi_mngr.h"

#define HM01B0_DRV_VERSION (0)
#define HM01B0_DRV_SUBVERSION (3)

#define HM01B0_DEFAULT_ADDRESS (0x24)

#define HM01B0_PIXEL_X_NUM (324)
#define HM01B0_PIXEL_Y_NUM (244)

#define HM01B0_REG_MODEL_ID_H (0x0000)
#define HM01B0_REG_MODEL_ID_L (0x0001)
#define HM01B0_REG_SILICON_REV (0x0002)
#define HM01B0_REG_FRAME_COUNT (0x0005)
#define HM01B0_REG_PIXEL_ORDER (0x0006)

#define HM01B0_REG_MODE_SELECT (0x0100)
#define HM01B0_REG_IMAGE_ORIENTATION (0x0101)
#define HM01B0_REG_SW_RESET (0x0103)
#define HM01B0_REG_GRP_PARAM_HOLD (0x0104)

#define HM01B0_REG_I2C_ID_SEL (0x3400)
#define HM01B0_REG_I2C_ID_REG (0x3401)

#define HM01B0_REG_PMU_PROGRAMMABLE_FRAMECNT (0x3020)

// #define HM01B0_REG_MODE_SELECT (0x0100)
#define HM01B0_REG_MODE_SELECT_STANDBY (0x00)
#define HM01B0_REG_MODE_SELECT_STREAMING (0x01)
#define HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES (0x03)
#define HM01B0_REG_MODE_SELECT_STREAMING_HW_TRIGGER (0x05)

// #define HM01B0_REG_IMAGE_ORIENTATION                    (0x0101)
#define HM01B0_REG_IMAGE_ORIENTATION_DEFAULT (0x00)
#define HM01B0_REG_IMAGE_ORIENTATION_HMIRROR (0x01)
#define HM01B0_REG_IMAGE_ORIENTATION_VMIRROR (0x02)
#define HM01B0_REG_IMAGE_ORIENTATION_HVMIRROR \
  (HM01B0_REG_IMAGE_ORIENTATION_HMIRROR | HM01B0_REG_IMAGE_ORIENTATION_HVMIRROR)

// #define HM01B0_REG_GRP_PARAM_HOLD                       (0x0104)
#define HM01B0_REG_GRP_PARAM_HOLD_CONSUME (0x00)
#define HM01B0_REG_GRP_PARAM_HOLD_HOLD (0x01)

#define HM01B0_LINE_WIDTH (324)
#define HM01B0_LINE_NUMBER (324)

typedef struct {
  uint16_t ui16Reg;
  uint8_t ui8Val;
} hm_script_t;

typedef enum {
    STANDBY   = HM01B0_REG_MODE_SELECT_STANDBY,
    STREAMING = HM01B0_REG_MODE_SELECT_STREAMING,
    STREAM_N  = HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES,
    HW_TRIG   = HM01B0_REG_MODE_SELECT_STREAMING_HW_TRIGGER
} hm01b0_mode;

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
void hm01b0_power_up(void);

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
void hm01b0_power_down(void);

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
void hm01b0_mclk_enable(void);

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
void hm01b0_mclk_disable(void);

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
int32_t hm01b0_init_if(const nrf_twi_mngr_t* instance);

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
int32_t hm01b0_deinit_if(void);

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
int32_t hm01b0_get_modelid(uint16_t *model_id);

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
                            uint32_t ui32ScriptCmdNum);

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
//int32_t hm01b0_test_walking1s(hm01b0_cfg_t *psCfg);

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
//void hm01b0_test_walking1s_check_data_sanity(uint8_t *pui8Buffer,
//                                             uint32_t ui32BufferLen,
//                                             uint32_t ui32PrintCnt);

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
int32_t hm01b0_reset_sw(void);

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
int32_t hm01b0_get_mode(uint8_t* mode);

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
//! @param framecnt     - Frame count for
//! HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES.
//!                     - Discarded if other modes.
//!
//! This function set HM01B0 operation mode.
//!
//! @return Error code.
//
//*****************************************************************************
int32_t hm01b0_set_mode(hm01b0_mode mode, uint8_t frame_cnt);

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
//int32_t hm01b0_hardware_trigger_streaming(hm01b0_cfg_t *psCfg, bool bTrigger);

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
//int32_t hm01b0_set_mirror(hm01b0_cfg_t *psCfg, bool bHmirror, bool bVmirror);

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
//int32_t hm01b0_blocking_read_oneframe(hm01b0_cfg_t *psCfg, uint8_t *pui8Buffer,
//                                       uint32_t ui32BufferLen);

//*****************************************************************************
//
//! @brief Read data of one frame from HM01B0.
//!
//! @param psCfg            - Pointer to HM01B0 configuration structure.
//!
//! This function wakes up the camera and captures a single frame.
//!
//! @return Error code.
//
//*****************************************************************************
//int32_t hm01b0_single_frame_capture(hm01b0_cfg_t *psCfg);

#ifdef __cplusplus
}
#endif

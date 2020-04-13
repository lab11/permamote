#pragma once

#include "nrf_twi_mngr.h"

#define HM01B0_DRV_VERSION (0)
#define HM01B0_DRV_SUBVERSION (3)

#define HM01B0_DEFAULT_ADDRESS (0x24)

#define HM01B0_PIXEL_X_NUM (324)
#define HM01B0_PIXEL_Y_NUM (324)
#define HM01B0_FULL_FRAME_PIXEL_X_NUM (320)
#define HM01B0_FULL_FRAME_PIXEL_Y_NUM (320)
#define HM01B0_RAW_IMAGE_SIZE (HM01B0_PIXEL_X_NUM * HM01B0_PIXEL_Y_NUM)
#define HM01B0_FULL_FRAME_IMAGE_SIZE (HM01B0_FULL_FRAME_PIXEL_X_NUM * HM01B0_FULL_FRAME_PIXEL_Y_NUM)
#define HM01B0_MCLK_FREQ (8E6)

#define HM01B0_REG_MODEL_ID_H (0x0000)
#define HM01B0_REG_MODEL_ID_L (0x0001)
#define HM01B0_REG_SILICON_REV (0x0002)
#define HM01B0_REG_FRAME_COUNT (0x0005)
#define HM01B0_REG_PIXEL_ORDER (0x0006)

#define HM01B0_REG_MODE_SELECT (0x0100)
#define HM01B0_REG_IMAGE_ORIENTATION (0x0101)
#define HM01B0_REG_SW_RESET (0x0103)
#define HM01B0_REG_GRP_PARAM_HOLD (0x0104)

#define HM01B0_REG_INTEGRATION_H  (0x0202)
#define HM01B0_REG_INTEGRATION_L  (0x0203)
#define HM01B0_REG_ANALOG_GAIN    (0x0205)
#define HM01B0_REG_DIGITAL_GAIN_H (0x020E)
#define HM01B0_REG_DIGITAL_GAIN_L (0x020F)

#define HM01B0_REG_AE_CTRL        (0x2100)

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

#define HM01B0_REG_GRP_PARAM_HOLD                       (0x0104)
#define HM01B0_REG_GRP_PARAM_HOLD_CONSUME (0x00)
#define HM01B0_REG_GRP_PARAM_HOLD_HOLD (0x01)

#define HM01B0_REG_FRAME_LENGTH_LINES_H 0x0340
#define HM01B0_REG_FRAME_LENGTH_LINES_L 0x0341

#define HM01B0_REG_LINE_LENGTH_PCK_H 0x0342
#define HM01B0_REG_LINE_LENGTH_PCK_L 0x0343

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
//! This function powers down HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_power_down(void);

//*****************************************************************************
//
//! @brief Initialize MCLK
//!
//! This function initializes a Timer, PPI, and GPIOTE to generate MCLK for HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_mclk_init(void);

//*****************************************************************************
//
//! @brief Enable MCLK
//!
//! This function enables the timer and PPI channel for MCLK
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_mclk_enable(void);

//*****************************************************************************
//
//! @brief Disable MCLK
//!
//! This function disable CTimer to stop MCLK for HM01B0.
//!
//! @return none.
//
//*****************************************************************************
void hm01b0_mclk_disable(void);

//*****************************************************************************
//
//! @brief Initialize I2C
//!
//! This function passes a pointer to the TWI manager
//!
//! @return err_code code.
//
//*****************************************************************************
void hm01b0_init_i2c(const nrf_twi_mngr_t* instance);

//*****************************************************************************
//
//! @brief Initialize interfaces
//!
//! This function initializes interfaces.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_init_if(void);

//*****************************************************************************
//
//! @brief Deinitialize interfaces
//!
//! This function deinitializes interfaces.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_deinit_if(void);

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
int32_t hm01b0_get_gain(uint8_t *again, uint8_t *dgainh, uint8_t *dgainl);

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
int32_t hm01b0_get_integration(uint16_t *integration);

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
int32_t hm01b0_set_integration(uint16_t integration);

//*****************************************************************************
//
//! @brief Enable HM01B0 Auto Exposure
//!
//! This function enables HM01B0 auto exposure.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_enable_ae(void);

//*****************************************************************************
//
//! @brief Disable HM01B0 Auto Exposure
//!
//! This function disables HM01B0 auto exposure.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_disable_ae(void);

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
int32_t hm01b0_get_line_pck_length(uint16_t *pck_length);

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
int32_t hm01b0_get_frame_lines_length(uint16_t *lines_length);

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
float hm01b0_get_exposure_time(uint16_t integration, uint16_t pck_length);

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
int32_t hm01b0_get_modelid(uint16_t *model_id);

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
                            uint32_t cmd_num);

//*****************************************************************************
//
//! @brief Software reset HM01B0
//!
//! This function resets HM01B0 by issuing a reset command.
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_reset_sw(void);

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
int32_t hm01b0_get_mode(uint8_t* mode);

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
int32_t hm01b0_set_mode(hm01b0_mode mode, uint8_t frame_cnt);

//*****************************************************************************
//
//! @brief Wait for sensor to set auto exposure
//!
//! This function waits for 5 exposures, allowing autogain settings to adjust
//!
//! @return err_code code.
//
//*****************************************************************************
int32_t hm01b0_wait_for_autoexposure(void);

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
int32_t hm01b0_blocking_read_oneframe(uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

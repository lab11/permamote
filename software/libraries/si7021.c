#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "nrf_drv_twi.h"
#include "nrf_delay.h"
#include "si7021.h"

#define Meas_RH_Hold_Master      0xE5  // Measure Relative Humidity, Hold Master Mode
#define Meas_RH_No_Hold_Master   0xF5  // Measure Relative Humidity, No Hold Master Mode
#define Meas_Temp_Hold_Master    0xE3  // Measure Temperature, Hold Master Mode
#define Meas_Temp_No_Hold_Master 0xF3  // Measure Temperature, No Hold Master Mode
#define Read_Temp_From_Prev_RH   0xE0  // Read Temperature Value from Previous RH Measurement
#define SI7021_RESET                    0xFE  // Reset
#define Write_User_Reg_1         0xE6  // Write RH/T User Register 1
#define Read_User_Reg_1          0xE7  // Read RH/T User Register 1
#define Read_ID_Byte_1           0xFA
#define Read_ID_Byte_1_b         0x0F  // Read 1st Electronic ID
#define Read_ID_Byte_2           0xFC  // Read 2nd Electronic ID
#define Read_ID_Byte_2_b         0xC9
#define Read_Firm_Rev_1           0x84  // Read Firmware Revision
#define Read_Firm_Rev_2          0xB8

#define HEATER_ON   0x04
#define HEATER_OFF  ~(HEATER_ON)

#define SI7021_ADDR  0x40

static const nrf_twi_mngr_t* twi_mngr_instance = NULL;

void si7021_reset(void) {
  uint8_t rst = SI7021_RESET;

  nrf_twi_mngr_transfer_t const reset_transfer[] = {
    NRF_TWI_MNGR_WRITE(SI7021_ADDR, &rst, 1, 0),
  };

  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, reset_transfer, 1, NULL);
  APP_ERROR_CHECK(error);

  nrf_delay_ms(20);
}

void si7021_init (const nrf_twi_mngr_t* instance) {
  twi_mngr_instance = instance;
}

void si7021_config (si7021_meas_res_t res_mode) {
  uint8_t res1, res0;
  uint8_t reg_status;
  si7021_read_user_reg(&reg_status);

  res1 = (res_mode & 0x2) << 6;
  res0 = res_mode & 0x1;

  uint8_t command[] = {
    Write_User_Reg_1,
    reg_status | res1 | res0
  };

  nrf_twi_mngr_transfer_t const config_transfer[] = {
    NRF_TWI_MNGR_WRITE(SI7021_ADDR, command, 2, 0),
  };

  int error;
  do {
    error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, config_transfer, 1, NULL);
  } while(error != NRF_SUCCESS);

}

void si7021_read_temp_hold (float* temp) {

  uint8_t command = Meas_Temp_Hold_Master;
  uint8_t temp_hum_data[3] = {0x00, 0x00, 0x00};

  nrf_twi_mngr_transfer_t const read_temp_hold_transfer[] = {
    NRF_TWI_MNGR_WRITE(SI7021_ADDR, &command, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(SI7021_ADDR, temp_hum_data, 3, 0),
  };
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_temp_hold_transfer, 2, NULL);
  APP_ERROR_CHECK(error);

  *temp = -46.85 + (175.72 * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xfc)) / (1 << 16));
}

//void si7021_read_temp (float* temp) {
//
//  uint8_t command = Meas_Temp_No_Hold_Master;
//  ret_code_t ret_code;
//
//  nrf_drv_twi_enable(m_instance);
//  nrf_drv_twi_tx(
//      m_instance,
//      SI7021_ADDR,
//      command,
//      sizeof(command),
//      true
//      );
//  uint8_t temp_hum_data[3] = {0, 0, 0};
//  do {
//    ret_code =
//      nrf_drv_twi_rx(
//          m_instance,
//          SI7021_ADDR,
//          temp_hum_data,
//          sizeof(temp_hum_data)
//          );
//  } while (ret_code != NRF_SUCCESS);
//  nrf_drv_twi_disable(m_instance);
//
//  *temp = -46.85 + (175.72 * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xfc)) / (1 << 16));
//}
//
void si7021_read_RH_hold (float* hum) {
  uint8_t command = Meas_RH_Hold_Master;
  uint8_t temp_hum_data[3] = {0, 0, 0};

  nrf_twi_mngr_transfer_t const read_rh_hold_transfer[] = {
    NRF_TWI_MNGR_WRITE(SI7021_ADDR, &command, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(SI7021_ADDR, temp_hum_data, 3, 0),
  };
  int error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_rh_hold_transfer, 2, NULL);
  APP_ERROR_CHECK(error);

  *hum = -6.0 + ((125.0 / (1 << 16)) * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xf0)));
}
//
//void si7021_read_RH (float* hum) {
//  read_RH(hum, Meas_RH_No_Hold_Master);
//}
//
//
void si7021_read_temp_after_RH (float* temp) {
  uint8_t command = Read_Temp_From_Prev_RH;
  uint8_t temp_hum_data[3] = {0, 0, 0};

  nrf_twi_mngr_transfer_t const read_temp_after_rh[] = {
    NRF_TWI_MNGR_WRITE(SI7021_ADDR, &command, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(SI7021_ADDR, temp_hum_data, 3, 0),
  };
  int error;
  do {
    error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_temp_after_rh, 2, NULL);
  }
  while(error != NRF_SUCCESS);

  *temp = -46.85 + (175.72 * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xfc)) / (1 << 16));
}
//
void si7021_read_temp_and_RH (float* temp, float* hum) {
  si7021_read_RH_hold(hum);
  si7021_read_temp_after_RH(temp); //employ read temp after RH shortcut
}
//
//
void si7021_read_user_reg (uint8_t* user_reg) {
  uint8_t command = Read_User_Reg_1;

  nrf_twi_mngr_transfer_t const read_user_reg_transfer[] = {
    NRF_TWI_MNGR_WRITE(SI7021_ADDR, &command, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(SI7021_ADDR, user_reg, 1, 0),
  };
  int error;
  do {
    error = nrf_twi_mngr_perform(twi_mngr_instance, NULL, read_user_reg_transfer, 2, NULL);
  } while (error != NRF_SUCCESS);
}

//
//void si7021_read_firmware_rev (uint8_t* buf) {
//
//  uint8_t command[2] = {
//    Read_Firm_Rev_1,
//    Read_Firm_Rev_2
//  };
//  nrf_drv_twi_enable(m_instance);
//  nrf_drv_twi_tx(
//      m_instance,
//      SI7021_ADDR,
//      command,
//      sizeof(command),
//      true
//      );
//  nrf_drv_twi_rx(
//      m_instance,
//      SI7021_ADDR,
//      buf,
//      sizeof(buf)
//      );
//  nrf_drv_twi_disable(m_instance);
//}
//
//void si7021_heater_on () {
//
//  uint8_t reg_status[1] = {0x00};
//  si7021_read_user_reg(reg_status);
//
//  uint8_t data[1] = {reg_status[0] | HEATER_ON};
//  uint8_t command[] = {
//    Write_User_Reg_1,
//    data[0]
//  };
//
//  nrf_drv_twi_enable(m_instance);
//  nrf_drv_twi_tx(
//      m_instance,
//      SI7021_ADDR,
//      command,
//      sizeof(command),
//      false
//      );
//  nrf_drv_twi_disable(m_instance);
//}
//
//void si7021_heater_off () {
//
//  uint8_t reg_status[1] = {0x00};
//  si7021_read_user_reg(reg_status);
//
//  uint8_t data[1] = {reg_status[0] & HEATER_OFF};
//  uint8_t command[] =  {
//    Write_User_Reg_1,
//    data[0]
//  };
//
//  nrf_drv_twi_enable(m_instance);
//  nrf_drv_twi_tx(
//      m_instance,
//      SI7021_ADDR,
//      command,
//      sizeof(command),
//      false
//      );
//  nrf_drv_twi_disable(m_instance);
//}

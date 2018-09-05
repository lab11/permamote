#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nrf_twi_mngr.h"

typedef enum {
    si7021_mode0, // RH = 12 bit, Temp = 14 bit
    si7021_mode1, // RH = 8 bit, Temp = 12 bit
    si7021_mode2, // RH = 10 bit, Temp = 13 bit
    si7021_mode3  // RH = 11 bit, Temp = 11 bit
} si7021_meas_res_t; //Measurement Resolution

void si7021_reset();

void si7021_init (const nrf_twi_mngr_t* instance);
void si7021_config(si7021_meas_res_t res_mode);

void si7021_read_temp_hold(float* temp);
//void si7021_read_temp(float* temp);
void si7021_read_RH_hold(float* hum);
//void si7021_read_RH(float* hum);
void si7021_read_temp_after_RH(float* temp);
void si7021_read_temp_and_RH(float* temp, float* hum);
//
void si7021_read_user_reg(uint8_t* user_reg);
//void si7021_read_firmware_rev(uint8_t* buf);
//
//void si7021_heater_on();
//void si7021_heater_off();

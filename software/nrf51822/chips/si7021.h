#pragma once

#define FIRMWARE_REV 1.0 // A10

typedef enum {
    si7021_MODE0, // RH = 12 bit, Temp = 14 bit
    si7021_MODE1, // RH = 8 bit, Temp = 12 bit
    si7021_MODE2, // RH = 10 bit, Temp = 13 bit
    so7021_MODE3  // RH = 11 bit, Temp = 11 bit
} si7021_meas_res_t; //Measurement Resolution

void si7021_reset();

void si7021_init(nrf_drv_twi_t * p_instance);
void si7021_config(si7021_meas_res_t res_mode);

void si7021_read_temp_hold(float* temp);
void si7021_read_temp(float* temp);
void si7021_read_RH_hold(float* hum);
void si7021_read_RH(float* hum);
void si7021_read_temp_after_RH(float* temp);
void si7021_read_temp_and_RH(float* temp, float* hum);

void si7021_read_user_reg(uint8_t* reg_status);
void si7021_read_firmware_rev(uint8_t* buf);

void si7021_heater_on();
void si7021_heater_off();

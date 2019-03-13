#pragma once
#include "nrf_drv_gpiote.h"

void rtc_update_callback(void);
void light_sensor_read_callback(float lux);
void light_interrupt_callback(void);
void discover_send_callback(void*);
void periodic_sensor_read_callback(void*);

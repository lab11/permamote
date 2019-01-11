#pragma once
#include "nrf_drv_gpiote.h"

void pir_interrupt_callback(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
void rtc_update_callback();
void light_sensor_read_callback(float lux);
void light_interrupt_callback(void);
void discover_send_callback(void*);
void periodic_sensor_read_callback(void*);
void pir_backoff_callback(void*);
void pir_enable_callback(void*);

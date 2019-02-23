#pragma once
#include "nrf_twi_mngr.h"
#include "nrf_drv_spi.h"
#include "app_timer.h"

void log_init(void);
void timer_init(void);
void twi_init(const nrf_twi_mngr_t * twi_instance);
void adc_init(void);
int sensors_init(const nrf_twi_mngr_t* twi_mngr_instance, const nrf_drv_spi_t* spi_instance);

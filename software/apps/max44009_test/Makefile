PROJECT_NAME = $(shell basename "$(realpath ./)")

APPLICATION_SRCS = $(notdir $(wildcard ./*.c))
APPLICATION_SRCS += softdevice_handler.c
APPLICATION_SRCS += ble_advdata.c
APPLICATION_SRCS += ble_conn_params.c
APPLICATION_SRCS += app_timer.c
APPLICATION_SRCS += app_error.c
APPLICATION_SRCS += ble_srv_common.c
APPLICATION_SRCS += app_util_platform.c
APPLICATION_SRCS += nrf_drv_common.c
APPLICATION_SRCS += nrf_drv_gpiote.c
APPLICATION_SRCS += app_gpiote.c
APPLICATION_SRCS += nrf_drv_clock.c
APPLICATION_SRCS += nrf_drv_twi.c
APPLICATION_SRCS += nrf_drv_uart.c
APPLICATION_SRCS += app_fifo.c
APPLICATION_SRCS += app_uart_fifo.c
APPLICATION_SRCS += retarget.c
APPLICATION_SRCS += led.c

APPLICATION_SRCS += simple_ble.c
APPLICATION_SRCS += simple_adv.c

APPLICATION_SRCS += max44009.c

NRF_IC = nrf51822
RAM_KB = 32
SOFTDEVICE_MODEL = s130
SDK_VERSION = 12

LIBRARY_PATHS += ./ ../../include/ ../../chips/
SOURCE_PATHS += ./ ../../chips/

NRF_BASE_PATH ?= ../../nrf5x-base
include $(NRF_BASE_PATH)/make/Makefile

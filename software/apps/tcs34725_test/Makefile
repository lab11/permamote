PROJECT_NAME = $(shell basename "$(realpath ./)")

APPLICATION_SRCS = $(notdir $(wildcard ./*.c))
APPLICATION_SRCS += app_error.c
APPLICATION_SRCS += app_error_weak.c
APPLICATION_SRCS += app_timer.c
APPLICATION_SRCS += app_util_platform.c
APPLICATION_SRCS += nrf_queue.c
APPLICATION_SRCS += nrfx_prs.c
APPLICATION_SRCS += nrfx_gpiote.c
APPLICATION_SRCS += nrfx_power.c
APPLICATION_SRCS += nrfx_uart.c
APPLICATION_SRCS += nrfx_uarte.c
APPLICATION_SRCS += nrfx_twi.c
APPLICATION_SRCS += nrf_twi_mngr.c
APPLICATION_SRCS += nrf_drv_uart.c
APPLICATION_SRCS += nrf_drv_twi.c
APPLICATION_SRCS += app_fifo.c
APPLICATION_SRCS += app_uart_fifo.c
APPLICATION_SRCS += retarget.c

APPLICATION_SRCS += tcs34725.c

NRF_IC = nrf52840_xxaa
SDK_VERSION = 15
#SOFTDEVICE_MODEL = s140

LIBRARY_PATHS += ./ ../../include/ ../../chips/
SOURCE_PATHS += ./ ../../chips/

NRF_BASE_PATH ?= ../../nrf5x-base
include $(NRF_BASE_PATH)/make/Makefile

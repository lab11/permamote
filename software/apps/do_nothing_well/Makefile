PROJECT_NAME = $(shell basename "$(realpath ./)")

APPLICATION_SRCS = $(notdir $(wildcard ./*.c))
APPLICATION_SRCS += app_timer.c
APPLICATION_SRCS += app_error.c
APPLICATION_SRCS += app_error_weak.c
APPLICATION_SRCS += nrf_section_iter.c
APPLICATION_SRCS += app_util_platform.c
APPLICATION_SRCS += nrf_pwr_mgmt.c
APPLICATION_SRCS += nrfx_clock.c
APPLICATION_SRCS += nrfx_spi.c
APPLICATION_SRCS += nrfx_uart.c
APPLICATION_SRCS += nrfx_power.c
APPLICATION_SRCS += app_fifo.c
APPLICATION_SRCS += app_uart_fifo.c
APPLICATION_SRCS += retarget.c

NRF_IC = nrf52840_xxaa

SDK_VERSION = 15
SOFTDEVICE_MODEL = s140
ifdef SOFTDEVICE_MODEL
  APPLICATION_SRCS += nrf_sdh.c
endif

LIBRARY_PATHS += ./ ../../include/ ../../chips/
SOURCE_PATHS += ./ ../../chips/

NRF_BASE_PATH ?= ../../nrf5x-base
include $(NRF_BASE_PATH)/make/Makefile

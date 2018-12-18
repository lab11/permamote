# Board-specific configurations for the nRF52 development kit (pca10040)

# Ensure that this file is only included once
ifndef BOARD_MAKEFILE
BOARD_MAKEFILE = 1

# Get directory of this makefile
BOARD_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# Include any files in this directory in the build process
BOARD_SOURCE_PATHS = $(BOARD_DIR)/.
BOARD_HEADER_PATHS = $(BOARD_DIR)/.
BOARD_LINKER_PATHS = $(BOARD_DIR)/.
BOARD_SOURCES = $(notdir $(wildcard $(BOARD_DIR)/./*.c))
BOARD_AS = $(notdir $(wildcard $(BOARD_DIR)/./*.s))

# Board-specific configurations
USE_BLE = 0

# Additional #define's to be added to code by the compiler
BOARD_VARS = \
	BOARD_CUSTOM\
	USE_APP_CONFIG\
	DEBUG\
	DEBUG_NRF\
	NRF_DFU_SETTINGS_VERSION=0\
	FLOAT_ABI_HARD\

# Default SDK source files to be included
BOARD_SOURCES += \
	app_scheduler.c\
	app_timer.c\
	app_util_platform.c\
	crc32.c\
	nrf_assert.c\
	nrf_atomic.c\
	nrf_balloc.c\
	nrf_fprintf.c\
	nrf_fprintf_format.c\
	nrf_fstorage.c\
	nrf_fstorage_nvmc.c\
	nrf_nvmc.c\
	nrf_log_backend_rtt.c\
	nrf_log_backend_serial.c\
	nrf_log_default_backends.c\
	nrf_log_frontend.c\
	nrf_log_str_formatter.c\
	nrf_crypto_ecc.c\
	nrf_crypto_ecdsa.c\
	nrf_crypto_hash.c\
	nrf_crypto_init.c\
	nrf_crypto_shared.c\
	cc310_bl_backend_ecc.c\
	cc310_bl_backend_ecdsa.c\
	cc310_bl_backend_init.c\
	cc310_bl_backend_hash.c\
	cc310_bl_backend_shared.c\
	nrf_memobj.c\
	nrf_ringbuf.c\
	nrf_strerror.c\
	nrf_queue.c\
	nrf_drv_clock.c\
	nrfx_gpiote.c\
	nrfx_timer.c\
	nrfx_clock.c\
	SEGGER_RTT.c\
	SEGGER_RTT_Syscalls_GCC.c\
	SEGGER_RTT_printf.c\
	nrf_bootloader.c\
	nrf_bootloader_app_start.c\
	nrf_bootloader_app_start_final.c\
	nrf_bootloader_dfu_timers.c\
	nrf_bootloader_fw_activation.c\
	nrf_bootloader_info.c\
	nrf_bootloader_wdt.c\
	pb_common.c\
	pb_decode.c\
	dfu-cc.pb.c\
	nrf_dfu.c\
	nrf_dfu_flash.c\
	nrf_dfu_handling_error.c\
	nrf_dfu_mbr.c\
	nrf_dfu_req_handler.c\
	nrf_dfu_settings.c\
	nrf_dfu_transport.c\
	nrf_dfu_utils.c\
	nrf_dfu_validation.c\
	nrf_dfu_ver_validation.c\

endif


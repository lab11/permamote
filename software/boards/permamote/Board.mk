# Board-specific configurations for the nRF52 development kit (pca10040)

# Ensure that this file is only included once
ifndef BOARD_MAKEFILE
BOARD_MAKEFILE = 1

# Get directory of this makefile
BOARD_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# Include any files in this directory in the build process
BOARD_SOURCE_PATHS = $(BOARD_DIR)/. $(BOARD_DIR)/../../libraries/
BOARD_HEADER_PATHS = $(BOARD_DIR)/.
BOARD_HEADER_PATHS += $(BOARD_DIR)/../../libraries/
BOARD_LINKER_PATHS = $(BOARD_DIR)/.
BOARD_SOURCES = $(notdir $(wildcard $(BOARD_DIR)/./*.c))
BOARD_SOURCES += $(notdir $(wildcard $(BOARD_DIR)/../../libraries/*.c))
BOARD_AS = $(notdir $(wildcard $(BOARD_DIR)/./*.s))

# Board-specific configurations
USE_BLE = 0

# Additional #define's to be added to code by the compiler
BOARD_VARS = \
	NRF_DFU_SETTINGS_VERSION=1\
	BOARD_CUSTOM\
	USE_APP_CONFIG\
	DEBUG\
	DEBUG_NRF\

# Default SDK source files to be included
BOARD_SOURCES += \
	app_error_handler_gcc.c\
	app_scheduler.c\
	app_timer.c\
	app_util_platform.c\
	before_startup.c\
	hardfault_handler_gcc.c\
	hardfault_implementation.c\
	nrf_assert.c\
	nrf_atomic.c\
	nrf_balloc.c\
	nrf_drv_twi.c\
	nrf_drv_spi.c\
	nrf_twi_mngr.c\
	nrf_spi_mngr.c\
	nrf_fprintf.c\
	nrf_fprintf_format.c\
	nrf_log_backend_rtt.c\
	nrf_log_backend_serial.c\
	nrf_log_default_backends.c\
	nrf_log_frontend.c\
	nrf_log_str_formatter.c\
	nrf_pwr_mgmt.c\
	nrf_memobj.c\
	mem_manager.c\
	nrf_ringbuf.c\
	nrf_section_iter.c\
	nrf_sdh.c\
	nrf_strerror.c\
	nrf_queue.c\
	nrf_drv_clock.c\
	nrf_nvmc.c\
	nrf_drv_ppi.c\
	nrfx_gpiote.c\
	nrfx_saadc.c\
	nrfx_timer.c\
	nrfx_twi.c\
	nrfx_twim.c\
	nrfx_spi.c\
	nrfx_spim.c\
	nrfx_clock.c\
	nrfx_ppi.c\
	nrfx_rtc.c\
	SEGGER_RTT.c\
	SEGGER_RTT_Syscalls_GCC.c\
	SEGGER_RTT_printf.c\
	device_id.c\
	simple_thread.c\
	thread_coap.c\
	thread_dns.c\
	thread_ntp.c\
	nrf_fstorage.c \
	nrf_fstorage_nvmc.c \
	nrf_crypto_ecc.c \
	nrf_crypto_ecdsa.c \
	nrf_crypto_hash.c \
	nrf_crypto_init.c \
	nrf_crypto_shared.c \
	sha256.c \
	cc310_bl_backend_hash.c \
	cc310_bl_backend_ecc.c \
	cc310_bl_backend_ecdsa.c \
	cc310_bl_backend_init.c \
	cc310_bl_backend_shared.c \
	coap.c\
	coap_block.c\
	coap_message.c\
	coap_option.c\
	coap_queue.c\
	coap_resource.c\
	coap_observe.c\
	coap_transport_ot.c\
	coap_dfu.c\
	crc32.c \
	pb_common.c\
	pb_decode.c\
	dfu-cc.pb.c \
	nrf_dfu_flash.c \
	nrf_dfu_settings.c \
	nrf_dfu_utils.c \
	nrf_dfu_req_handler.c \
	nrf_dfu_handling_error.c \
	nrf_dfu_validation.c \
	nrf_dfu_ver_validation.c \
	background_dfu_block.c \
	background_dfu_operation.c \
	background_dfu_state.c \
	fds.c \
	nrf_atfifo.c \

endif

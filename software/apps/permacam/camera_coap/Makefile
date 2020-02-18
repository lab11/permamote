PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52840
SDK_VERSION = 16
SOFTDEVICE_MODEL = blank
USE_THREAD = 1
USE_BOOTLOADER = 1
PROTO_DIR = ../../../../docs/gateway/

SDK_VARS += __HEAP_SIZE=0x20000

# Source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .
APP_SOURCES = $(notdir $(wildcard ./*.c))
APP_SOURCES += $(notdir $(wildcard ./*.proto))
APP_SOURCES += parse.proto

NRF_BASE_DIR ?= ../../../nrf52x-base
LINKER_SCRIPT = $(NRF_BASE_DIR)/make/ld/gcc_nrf52840_dfu_blank_0_256_1024.ld
# Include board Makefile (if any)
include $(NRF_BASE_DIR)/../boards/permacam/Board.mk
# Include main Makefile
include $(NRF_BASE_DIR)/make/AppMakefile.mk

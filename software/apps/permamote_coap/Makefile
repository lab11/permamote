PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52840
SDK_VERSION = 16
SOFTDEVICE_MODEL = blank
USE_THREAD = 1
USE_GATEWAY = 1
USE_BOOTLOADER = 1
BOOTLOADER = bg_bootloader
PROTO_DIR = ../../../docs/gateway/

# Source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .
APP_SOURCES = $(notdir $(wildcard ./*.c))
APP_SOURCES += $(notdir $(wildcard ./*.proto))
APP_SOURCES += parse.proto

NRF_BASE_DIR ?= ../../nrf52x-base

# Include board Makefile (if any)
include $(NRF_BASE_DIR)/../boards/permamote/Board.mk
# Include main Makefile
include $(NRF_BASE_DIR)/make/AppMakefile.mk

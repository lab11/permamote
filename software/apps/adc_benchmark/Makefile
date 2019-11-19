PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52840
SDK_VERSION = 15
SOFTDEVICE_MODEL = blank
USE_THREAD = 1
#USE_BOOTLOADER = 1

# Source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .
APP_SOURCES = $(notdir $(wildcard ./*.c))

NRF_BASE_DIR ?= ../../nrf52x-base
#LINKER_SCRIPT = $(NRF_BASE_DIR)/make/ld/gcc_nrf52840_dfu_blank_0_256_1024.ld
# Include board Makefile (if any)
include $(NRF_BASE_DIR)/../boards/permamote/Board.mk
# Include main Makefile
include $(NRF_BASE_DIR)/make/AppMakefile.mk

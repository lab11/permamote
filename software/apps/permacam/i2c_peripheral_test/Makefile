PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52840
SDK_VERSION = 16
SOFTDEVICE_MODEL = blank
USE_THREAD = 1
USE_GATEWAY = 0
PROTO_DIR = ../../../../docs/gateway/

SDK_VARS += __STACK_SIZE=0x25000

# Source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .

NRF_BASE_DIR ?= ../../../nrf52x-base
# Include board Makefile (if any)
include $(NRF_BASE_DIR)/../boards/permacam/Board.mk
# Include main Makefile
include $(NRF_BASE_DIR)/make/AppMakefile.mk

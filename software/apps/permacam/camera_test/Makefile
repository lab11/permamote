PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52840
SDK_VERSION = 16
SOFTDEVICE_MODEL = blank
USE_THREAD = 1
PROTO_DIR = ../../../../docs/gateway/

SDK_VARS += __STACK_SIZE=0x10000 __HEAP_SIZE=0x25000

# Source and header files
APP_HEADER_PATHS += .
APP_HEADER_PATHS += ../../../libraries/jpec/include
APP_HEADER_PATHS += ../../../libraries/jpec/src
APP_SOURCE_PATHS += .
APP_SOURCE_PATHS += ../../../libraries/jpec/src
APP_SOURCES = $(notdir $(wildcard ./*.c))
APP_SOURCES += $(wildcard ../../../libraries/jpec/src/*.c)
APP_SOURCES += parse.proto

NRF_BASE_DIR ?= ../../../nrf52x-base
# Include board Makefile (if any)
include $(NRF_BASE_DIR)/../boards/permacam/Board.mk
# Include main Makefile
include $(NRF_BASE_DIR)/make/AppMakefile.mk

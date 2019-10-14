PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52840
SDK_VERSION = 15
SOFTDEVICE_MODEL = blank
USE_THREAD = 1
USE_BOOTLOADER = 1

# Source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .
APP_SOURCES = $(notdir $(wildcard ./*.c))

NRF_BASE_DIR ?= ../../nrf52x-base
LINKER_SCRIPT = $(NRF_BASE_DIR)/make/ld/gcc_nrf52840_dfu_blank_0_256_1024.ld

# Include board Makefile (if any)
include $(NRF_BASE_DIR)/../boards/permamote/Board.mk
# Include main Makefile
include $(NRF_BASE_DIR)/make/AppMakefile.mk

# TODO: make this a platform/version independent process!
proto:
	protoc -I/usr/local/Cellar/protobuf/3.10.0/include -I/usr/local/Cellar/nanopb-generator/0.3.9.3/libexec/proto/ -I. --nanopb_out=. permamote_message.proto
clean-proto:
	rm permamote_message.pb*

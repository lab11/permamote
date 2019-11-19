// application-specific configuration
// augments the base configuration in sdk/<SDK>/config/<IC>/config/sdk_config.h

#pragma once

#define BLE_ADVERTISING_ENABLED 0
#define NRF_BLE_CONN_PARAMS_ENABLED 0
#define NRF_BLE_GATT_ENABLED 0
#define NRF_BLE_QWR_ENABLED 0
#define BLE_ECS_ENABLED 0

#define BLE_LBS_ENABLED 0
#define BLE_NUS_ENABLED 0

#define NRF_CRYPTO_BACKEND_CIFRA_ENABLED 1
#define NRF_CRYPTO_BACKEND_MBEDTLS_ENABLED 0
#define NRF_CRYPTO_BACKEND_NRF_HW_RNG_ENABLED 1

#define GPIOTE_ENABLED 1
#define GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 4
#define NRFX_GPIOTE_ENABLED 1

#define NRFX_RNG_ENABLED 1
#define RNG_ENABLED 1

#define NRFX_SAADC_ENABLED 1
#define SAADC_ENABLED 1
#define SAADC_CONFIG_LP_MODE 1

#define NRFX_PRS_ENABLED 0

#define NRFX_SPIM_ENABLED 1
#define NRFX_SPI_ENABLED 1
#define SPI_ENABLED 1
#define SPI1_ENABLED 1
#define NRF_SPI_MNGR_ENABLED 1

#define APP_SDCARD_ENABLED 1

#define NRF_CLOCK_ENABLED 1
#define NRFX_CLOCK_ENABLED 1
#define CLOCK_CONFIG_LF_SRC 1
#define NRFX_TIMER_ENABLED 1
#define TIMER_ENABLED 1
#define TIMER_DEFAULT_CONFIG_BIT_WIDTH 3
#define TIMER0_ENABLED 1
#define TIMER3_ENABLED 1
#define NRFX_RTC_ENABLED 1
#define RTC_ENABLED 1
#define RTC0_ENABLED 1
#define APP_TIMER_ENABLED 1
#define APP_TIMER_CONFIG_USE_SCHEDULER 0
#define APP_TIMER_KEEPS_RTC_ACTIVE 1

#define PPI_ENABLED 1
#define NRFX_PPI_ENABLED 1

#define NRFX_UARTE_ENABLED 0
#define NRFX_UART_ENABLED 0
#define UART_ENABLED 0
#define UART1_ENABLED 0
#define APP_UART_ENABLED 0

#define NRFX_TWIM_ENABLED 1
#define NRFX_TWI_ENABLED 1
#define TWI_ENABLED 1
#define NRFX_TWI0_ENABLED 1
#define TWI0_ENABLED 1
#define TWI0_USE_EASY_DMA 0
#define NRF_TWI_MNGR_ENABLED 1

#define APP_FIFO_ENABLED 1
#define APP_SCHEDULER_ENABLED 1

#define CRC16_ENABLED 1

#define NRF_FSTORAGE_ENABLED 1
#define FDS_ENABLED 1
#define FDS_VIRTUAL_PAGES 8
#define FDS_OP_QUEUE_SIZE 5
#define FDS_VIRTUAL_PAGES_RESERVED 4
#define FDS_BACKEND 1

#define MEM_MANAGER_ENABLED 1
#define MEMORY_MANAGER_SMALL_BLOCK_COUNT 8
#define MEMORY_MANAGER_SMALL_BLOCK_SIZE 128
#define MEMORY_MANAGER_MEDIUM_BLOCK_COUNT 8
#define MEMORY_MANAGER_MEDIUM_BLOCK_SIZE 256
#define MEMORY_MANAGER_LARGE_BLOCK_COUNT 8
#define MEMORY_MANAGER_LARGE_BLOCK_SIZE 1024

#define NRF_QUEUE_ENABLED 1

#define NRF_PWR_MGMT_ENABLED 1
#define NRF_PWR_MGMT_CONFIG_FPU_SUPPORT_ENABLED 1

#define RETARGET_ENABLED 0

#define BUTTON_ENABLED 0

#define NRF_SECTION_ITER_ENABLED 0

#define NRF_LOG_ENABLED 1
#define NRF_LOG_BACKEND_UART_ENABLED 0
#define NRF_LOG_BACKEND_RTT_ENABLED 1
#define NRF_LOG_USES_RTT 1
#define NRF_LOG_DEFERRED 0
#define NRF_LOG_STR_PUSH_BUFFER_SIZE 32

#define HARDFAULT_HANDLER_ENABLED 1
#define HARDFAULT_HANDLER_GDB_PSP_BACKTRACE 0

#define NRF_SERIAL_ENABLED 1

#define NRF_SDH_ENABLED 0
#define NRF_SDH_BLE_ENABLED 0
#define NRF_SDH_BLE_GAP_DATA_LENGTH 251
#define NRF_SDH_BLE_PERIPHERAL_LINK_COUNT 1
#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE 247
#define NRF_SDH_BLE_VS_UUID_COUNT 10
#define NRF_SDH_SOC_ENABLED 0

#define NRF_CRYPTO_ENABLED 1
#define NRF_CRYPTO_BACKEND_CC310_BL_ENABLED 1
#define NRF_CRYPTO_BACKEND_CC310_BL_INTERRUPTS_ENABLED 1
#define NRF_CRYPTO_BACKEND_MBEDTLS_ECC_SECP256R1_ENABLED 0
#define NRF_CRYPTO_BACKEND_CC310_BL_ECC_SECP256R1_ENABLED 1
#define NRF_CRYPTO_BACKEND_CC310_BL_HASH_SHA256_ENABLED 1
#define NRF_CRYPTO_BACKEND_CC310_BL_HASH_AUTOMATIC_RAM_BUFFER_SIZE 4096
#define NRF_CRYPTO_BACKEND_CC310_BL_HASH_AUTOMATIC_RAM_BUFFER_ENABLED 1

#define CRC32_ENABLED 1

#define NRF_DFU_DEBUG_VERSION 1
#define DFU_RESOURCE_PREFIX "dfu"
#define DFU_UDP_PORT 5683
#define NRF_DFU_TRANSPORT_BLE 0
#define NRF_DFU_PROTOCOL_FW_VERSION_MSG 1
#define NRF_DFU_PROTOCOL_VERSION_MSG 1
#define NRF_DFU_APP_DOWNGRADE_PREVENTION 1
#define NRF_DFU_FORCE_DUAL_BANK_APP_UPDATES 1
#define NRF_DFU_HW_VERSION 52
#define NRF_DFU_REQUIRE_SIGNED_APP_UPDATE 1
#define NRF_DFU_SINGLE_BANK_APP_UPDATES 0
#define NRF_DFU_APP_DATA_AREA_SIZE ((FDS_VIRTUAL_PAGES_RESERVED + FDS_VIRTUAL_PAGES) * FDS_VIRTUAL_PAGE_SIZE * 4)
#define NRF_DFU_SAVE_PROGRESS_IN_FLASH 0
#define NRF_DFU_SETTINGS_ALLOW_UPDATE_FROM_APP 1
#define NRF_DFU_IN_APP 1
#define NRF_DFU_SETTINGS_COMPATIBILITY_MODE 0
#define NRF_BL_APP_SIGNATURE_CHECK_REQUIRED 1
#define BACKGROUND_DFU_DEFAULT_BLOCK_SIZE 512
#define BACKGROUND_DFU_BLOCKS_PER_BUFFER 64
#define BACKGROUND_DFU_CONFIG_LOG_LEVEL 3
#define BACKGROUND_DFU_RETRIES         5
#define MEM_MANAGER_CONFIG_LOG_ENABLED 1
#define IOT_COAP_CONFIG_LOG_ENABLED 1
#define COAP_VERSION 1
#define COAP_MESSAGE_QUEUE_SIZE 8
#define COAP_ACK_RANDOM_FACTOR 1.5
#define COAP_MAX_RETRANSMIT_COUNT 4
#define COAP_ACK_TIMEOUT 2
#define COAP_MAX_TRANSMISSION_SPAN 45
#define COAP_ENABLE_OBSERVE_CLIENT 1
#define COAP_OBSERVE_MAX_NUM_OBSERVABLES 2

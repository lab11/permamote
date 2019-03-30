/** @file
 *
 * @defgroup bootloader_secure main.c
 * @{
 * @ingroup dfu_bootloader_api
 * @brief Bootloader project main file for secure DFU.
 *
 */

#include <stdint.h>
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_soc.h"
#include "nrf_dfu.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_bootloader_info.h"
#include "nrf_gpio.h"
#include "nrf_drv_clock.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_dfu_mbr.h"

#include "custom_board.h"

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("received a fault! id: 0x%08x, pc: 0x&08x\r\n", id, pc);
    NVIC_SystemReset();
}

void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("received an error: 0x%08x!\r\n", error_code);
    NVIC_SystemReset();
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("received an error: 0x%08x, line: 0x%lu, file: %s\r\n", error_code, line_num, p_file_name);
    NVIC_SystemReset();
}

/**
 * @brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**
 * @brief Function notifies certain events in DFU process.
 */
static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    switch (evt_type)
    {
        case NRF_DFU_EVT_DFU_INITIALIZED:
            nrf_gpio_pin_clear(LED_1);
            break;

        default:
            break;
    }
}

/**@brief Function for application main entry.
 */
int main(void)
{
    uint32_t ret_val;

    //ret_code_t err_code = nrf_drv_clock_init();
    //APP_ERROR_CHECK(err_code);
    //nrf_drv_clock_lfclk_request(NULL);

    log_init();

    NRF_LOG_INFO("Inside main\r\n");

    nrf_gpio_cfg_output(LED_1);
    nrf_gpio_pin_set(LED_1);

    ret_val = nrf_bootloader_init(dfu_observer);
    APP_ERROR_CHECK(ret_val);

    while(1){}

    // Either there was no DFU functionality enabled in this project or the DFU module detected
    // no ongoing DFU operation and found a valid main application.
    // Boot the main application.
    nrf_bootloader_app_start();

    // Should never be reached.
    NRF_LOG_INFO("After main\r\n");

}

/**
 * @}
 */

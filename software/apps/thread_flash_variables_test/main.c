/* Flash Variables Test
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf.h"
#include "nrf_timer.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_power.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "simple_thread.h"

#include "nrf_fstorage.h"
#include "nrf_fstorage_nvmc.h"

#define DEFAULT_CHILD_TIMEOUT    40                                         /**< Thread child timeout [s]. */
#define DEFAULT_POLL_PERIOD      1000                                       /**< Thread Sleepy End Device polling period when MQTT-SN Asleep. [ms] */
#define NUM_SLAAC_ADDRESSES      4                                          /**< Number of SLAAC addresses. */

// typedef int mutable_integer_parameter_t;

// NRF_SECTION_DEF(mutable_flash_section, mutable_integer_parameter_t);

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

// void callback(nrf_fstorage_evt_t * p_evt) {
//     /* do nothing */
// }
// NRF_FSTORAGE_DEF(nrf_fstorage_t my_instance) =
// {
//     .evt_handler    = callback,
//     .start_addr     = 0xFD000,
//     .end_addr       = 0xFFFFF,
// };

int main(void) {

    int int_inside_main = 1;


    nrf_power_dcdcen_set(1);

    log_init();

    //uint32_t* bootloader_start_address_ptr = (uint32_t*) 0x10001014;

    
    NRF_LOG_INFO("int_inside_main address: %x", &int_inside_main);

    //NRF_LOG_INFO("Reading data at location 0x10001014: %x", *bootloader_start_address_ptr);

    
    // nrf_fstorage_init(
    //     &my_instance,       /* You fstorage instance, previously defined. */
    //     &nrf_fstorage_nvmc,   /* Name of the backend. */
    //     NULL                /* Optional parameter, backend-dependant. */
    // );

    // static uint32_t number = 123;

    // ret_code_t rc = nrf_fstorage_write(
    //     &my_instance,   /* The instance to use. */
    //     0xFD000,     /* The address in flash where to store the data. */
    //     &number,        /* A pointer to the data. */
    //     sizeof(number), /* Lenght of the data, in bytes. */
    //     NULL            /* Optional parameter, backend-dependent. */
    // );

    // if (rc == NRF_SUCCESS)
    // {
    //     /* The operation was accepted.
    //     Upon completion, the NRF_FSTORAGE_WRITE_RESULT event
    //     is sent to the callback function registered by the instance. */
    //     NRF_LOG_INFO("Success");
    // }
    // else
    // {
    //     /* Handle error.*/
    //     NRF_LOG_INFO("Failure");
    // }
}

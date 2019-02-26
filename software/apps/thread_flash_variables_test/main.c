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

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}
void callback(nrf_fstorage_evt_t * p_evt) {
    /* do nothing */
}

NRF_FSTORAGE_DEF(nrf_fstorage_t my_instance) =
{
    .evt_handler    = callback,
    .start_addr     = 0xE4EC,
    .end_addr       = 0xF4EC,
};

//const int32_t flash_buffer[4] = {100, 0, 0, 0};


int main(void) {
    nrf_power_dcdcen_set(1);
    log_init();
    
    nrf_fstorage_init(
        &my_instance,       /* You fstorage instance, previously defined. */
        &nrf_fstorage_nvmc,   /* Name of the backend. */
        NULL                /* Optional parameter, backend-dependant. */
    );

    const int32_t *buffer_start_address = 0xE52C;//&(flash_buffer[0]);

    NRF_LOG_INFO("Buffer start address is: %p", buffer_start_address);
    NRF_LOG_INFO("Buffer first element (hex): %x", *buffer_start_address);
    NRF_LOG_INFO("Buffer first element (dec): %d", *buffer_start_address);
    
    static int32_t number = 12345;

    ret_code_t rc = nrf_fstorage_write(
        &my_instance,   /* The instance to use. */
        (uint32_t) buffer_start_address,     /* The address in flash where to store the data. */
        &number,        /* A pointer to the data. */
        sizeof(number), /* Lenght of the data, in bytes. */
        NULL            /* Optional parameter, backend-dependent. */
    );

    if (rc == NRF_SUCCESS)
    {
        /* The operation was accepted.
        Upon completion, the NRF_FSTORAGE_WRITE_RESULT event
        is sent to the callback function registered by the instance. */
        NRF_LOG_INFO("Success");
    }
    else
    {
        /* Handle error.*/
        NRF_LOG_INFO("Failure");
    }

    NRF_LOG_INFO("Buffer first element (hex): %x", *buffer_start_address);
    NRF_LOG_INFO("Buffer first element (dec): %d", *buffer_start_address);

    static int32_t retrieve_number;
    NRF_LOG_INFO("Retrieval value starts as (hex): %x", retrieve_number);
    NRF_LOG_INFO("Retrieval value starts as (dec): %d", retrieve_number);

    rc = nrf_fstorage_read(
        &my_instance,   /* The instance to use. */
        (uint32_t) buffer_start_address,     /* The address in flash where to read data from. */
        &retrieve_number,        /* A buffer to copy the data into. */
        sizeof(retrieve_number)  /* Lenght of the data, in bytes. */
    );

    if (rc == NRF_SUCCESS)
    {
        /* The operation was accepted.
        Upon completion, the NRF_FSTORAGE_READ_RESULT event
        is sent to the callback function registered by the instance.
        Once the event is received, it is possible to read the contents of 'number'. */
        NRF_LOG_INFO("Success");
    }
    else
    {
        /* Handle error.*/
        NRF_LOG_INFO("Failure");
    }

    //NRF_LOG_INFO("Buffer first element is (hex): %x", *buffer_start_address);
    //NRF_LOG_INFO("Buffer first element is (dec): %d", *buffer_start_address);
    NRF_LOG_INFO("Retrieval value is now (hex): %x", retrieve_number);
    NRF_LOG_INFO("Retrieval value is now (dec): %d", retrieve_number);
}

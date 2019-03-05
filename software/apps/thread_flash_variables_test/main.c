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


#define FLASH_ADDR A18

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


void callback(nrf_fstorage_evt_t * p_evt) {
    NRF_LOG_INFO("Callback is being called");
    if (p_evt->id == NRF_FSTORAGE_EVT_WRITE_RESULT) {
        NRF_LOG_INFO("Write Result");
    }
    if (p_evt->result == NRF_SUCCESS) {
        NRF_LOG_INFO("id: %x", p_evt->id);
        NRF_LOG_INFO("result: %x", p_evt->result);
        NRF_LOG_INFO("addr: %x", p_evt->addr);
        NRF_LOG_INFO("p_src: %x", p_evt->p_src);
        NRF_LOG_INFO("len: %x", p_evt->len);
        NRF_LOG_INFO("p_param: %x", p_evt->p_param);
    }
}

NRF_FSTORAGE_DEF(nrf_fstorage_t my_instance) =
{
    .evt_handler    = callback,
    .start_addr     = 0xA18,
    .end_addr       = 0xFFC,
};

//const uint32_t flash_buffer[4] = {555, 0, 0, 0};


int main(void) {
    nrf_power_dcdcen_set(1);
    log_init();
    
    nrf_fstorage_init(
        &my_instance,       /* You fstorage instance, previously defined. */
        &nrf_fstorage_nvmc,   /* Name of the backend. */
        NULL                /* Optional parameter, backend-dependant. */
    );

    p = (uint32_t *) FLASH_ADDR;

    //const uint32_t *buffer_start_address = &(flash_buffer[0]);

    // uint32_t *buffer_start_address = (uint32_t *) 0xFF0;

    // //NRF_LOG_INFO("Buffer start address is: %p", buffer_start_address);
    // NRF_LOG_INFO("Buffer first element (hex): %x", *buffer_start_address);
    // NRF_LOG_INFO("Buffer first element (dec): %u", *buffer_start_address);

    NRF_LOG_INFO("Flash Address: %x", p);
    NRF_LOG_INFO("Flash Address contains: %x", *p)

    // uint32_t *p = (uint32_t *) 0xF0;
    // while(true) {
    //     NRF_LOG_INFO("%x: %x", p, *p)
    //     p++;
    //     //break;
    //     nrf_delay_ms(100);
    // }

    
//     uint32_t number = 0xFFF000FF;
    
//     ret_code_t rc;

//     rc = nrf_fstorage_write(
//         &my_instance,   /* The instance to use. */
//         (uint32_t) buffer_start_address,     /* The address in flash where to store the data. */
//         &number,        /* A pointer to the data. */
//         sizeof(number), /* Lenght of the data, in bytes. */
//         NULL            /* Optional parameter, backend-dependent. */
//     );

//     if (rc == NRF_SUCCESS)
//     {
//         /* The operation was accepted.
//         Upon completion, the NRF_FSTORAGE_WRITE_RESULT event
//         is sent to the callback function registered by the instance. */
//         NRF_LOG_INFO("Write Success");
//     }
//     else
//     {
//         /* Handle error.*/
//         NRF_LOG_INFO("Write Failure");
//     }

//    //nrf_delay_ms(2000);

//     NRF_LOG_INFO("Buffer first element (hex): %x", *buffer_start_address);
//     NRF_LOG_INFO("Buffer first element (dec): %u", *buffer_start_address);
//     nrf_delay_ms(1000);

//     uint32_t retrieve_number = 0;
//     // NRF_LOG_INFO("Retrieval value starts as (hex): %x", retrieve_number);
//     // NRF_LOG_INFO("Retrieval value starts as (dec): %u", retrieve_number);

//     rc = nrf_fstorage_read(
//         &my_instance,   /* The instance to use. */
//         (uint32_t) buffer_start_address,     /* The address in flash where to read data from. */
//         &retrieve_number,        /* A buffer to copy the data into. */
//         sizeof(number)  /* Lenght of the data, in bytes. */
//     );

//     if (rc == NRF_SUCCESS)
//     {
//         /* The operation was accepted.
//         Upon completion, the NRF_FSTORAGE_READ_RESULT event
//         is sent to the callback function registered by the instance.
//         Once the event is received, it is possible to read the contents of 'number'. */
//         NRF_LOG_INFO("Read Success");
//     }
//     else
//     {
//         /* Handle error.*/
//         NRF_LOG_INFO("Read Failure");
//     }

//     nrf_delay_ms(4000);

//     //NRF_LOG_INFO("Buffer first element is (hex): %x", *buffer_start_address);
//     //NRF_LOG_INFO("Buffer first element is (dec): %d", *buffer_start_address);
//     NRF_LOG_INFO("Retrieval value is now (hex): %x", retrieve_number);
//     NRF_LOG_INFO("Retrieval value is now (dec): %u", retrieve_number);
}

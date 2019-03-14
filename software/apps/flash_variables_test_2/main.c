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

#include "fds.h"
#include "nrf_atfifo.h"

//#include "nrf_fstorage.h"
//#include "nrf_fstorage_nvmc.h"

#define DEFAULT_CHILD_TIMEOUT    40 /**< Thread child timeout [s]. */
#define DEFAULT_POLL_PERIOD      1000 /**< Thread Sleepy End Device polling period when MQTT-SN Asleep. [ms] */
#define NUM_SLAAC_ADDRESSES      4 /**< Number of SLAAC addresses. */

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void) {
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void fds_evt_handler(fds_evt_t const * p_fds_evt) {
    switch (p_fds_evt->id) {
        case FDS_EVT_INIT:
            if (p_fds_evt->result != FDS_SUCCESS) {
                // Initialization failed.
            }
            break;
        default:
            break;
    }
}

static ret_code_t fds_write(uint16_t file_id, uint16_t record_key, void const *p_data, size_t data_size) {
    fds_record_t record;
    fds_record_desc_t record_desc;
    record.file_id = file_id;
    record.key = record_key;
    record.data.p_data = p_data;
    record.data.length_words = (data_size + 3) / 4; // 4 bytes = 1 word; we take the ceiling of the # of words
    return fds_record_write(&record_desc, &record);
}

static uint32_t fds_read(uint16_t file_id, uint16_t record_key) {
// right now this assumes all data will be read back out as uint32's.
// Also only ever returns the first thing found with the matching file id and record key
    uint32_t return_value = 0;

    fds_flash_record_t flash_record;
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;
    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token

    if (fds_record_find(file_id, record_key, &record_desc, &ftok) == FDS_SUCCESS) {
        NRF_LOG_INFO("Data Found!");

        if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS) {
            NRF_LOG_INFO("FDS read open failed");
        } else {
        return_value = *((uint32_t *)flash_record.p_data);
        }
        if (fds_record_close(&record_desc) != FDS_SUCCESS) {
             NRF_LOG_INFO("FDS read close failed");
        }
    }

    return return_value;
}

static ret_code_t flash_write(char *name, void const *p_data, size_t data_size) {

    return fds_write(0x1111, 0x1111, p_data, data_size);
}

static uint32_t flash_read(char *name) {
    return 0;
}


int main(void) {
    nrf_power_dcdcen_set(1);
    log_init();
    NRF_LOG_INFO("Start");

    //-------------------------------------------Setup-------------------------------------------

    ret_code_t rc;
    rc = fds_register(fds_evt_handler);
    if (rc != FDS_SUCCESS) {
        // Registering of the FDS event handler has failed.
        NRF_LOG_INFO("FDS register failed");
    } else {
        NRF_LOG_INFO("FDS register OK");
    }
    
    rc = fds_init();
    if (rc != FDS_SUCCESS) {
        NRF_LOG_INFO("FDS init failed");
    } else {
        NRF_LOG_INFO("FDS init OK");
    }

    //-------------------------------------------Write 1-------------------------------------------

    // uint16_t FILE_ID = 0x0001;  /* The ID of the file to write the records into. */
    // uint16_t RECORD_KEY_1 = 0x9999;  /* A key for the first record. */
    // uint16_t RECORD_KEY_2 = 0x2222;  /* A key for the second record. */
    // static uint32_t const m_deadbeef = 0xDEADBEEF;
    // static char const m_hello[]  = "Hello, world!";
    // fds_record_t record;
    // fds_record_desc_t record_desc_1;

    // Set up record.
    // record.file_id = FILE_ID;
    // record.key = RECORD_KEY_1;
    // record.data.p_data = &m_deadbeef;
    // record.data.length_words = 1;   /* one word is four bytes. */
    
    // rc = fds_record_write(&record_desc_1, &record);
    // if (rc != FDS_SUCCESS) {
    //     NRF_LOG_INFO("FDS write 1 failed");
    // } else {
    //     NRF_LOG_INFO("FDS write 1 OK");
    // }

    //-------------------------------------------Write 2-------------------------------------------

    // fds_record_desc_t record_desc_2;

    // // Set up record.
    // record.file_id = FILE_ID;
    // record.key = RECORD_KEY_2;
    // record.data.p_data = &m_hello;
    // /* The following calculation takes into account any eventual remainder of the division. */
    // record.data.length_words = (sizeof(m_hello) + 3) / 4;
    // rc = fds_record_write(&record_desc_2, &record);
    // if (rc != FDS_SUCCESS) {
    //     NRF_LOG_INFO("FDS write 2 failed");
    // } else {
    //     NRF_LOG_INFO("FDS write 2 OK");
    // }

    //-------------------------------------------Read 1-------------------------------------------

    // fds_flash_record_t flash_record;
    // fds_record_desc_t record_desc_3;
    // fds_find_token_t ftok;

    // /* It is required to zero the token before first use. */
    // memset(&ftok, 0x00, sizeof(fds_find_token_t));

    // /* Loop until all records with the given key and file ID have been found. */
    // while (fds_record_find(FILE_ID, RECORD_KEY_1, &record_desc_3, &ftok) == FDS_SUCCESS) {

    //     if (fds_record_open(&record_desc_3, &flash_record) != FDS_SUCCESS) {
    //         /* Handle error. */
    //         NRF_LOG_INFO("FDS read 1 open failed");
    //     }
    //     /* Access the record through the flash_record structure. */
    //     NRF_LOG_INFO("Record ID is: %x", (flash_record.p_header)->record_id);
    //     NRF_LOG_INFO("Record's data pointer points to: %x", *( (uint32_t *) flash_record.p_data));
    //     /* Close the record when done. */
    //     if (fds_record_close(&record_desc_3) != FDS_SUCCESS) {
    //         /* Handle error. */
    //          NRF_LOG_INFO("FDS read 1 close failed");
    //     }
    // }

    //-------------------------------------------Continued Testing with new fxns-------------
    int data_to_store = -1985;
    uint16_t data_file = 0x1235;
    uint16_t data_id = 0x5679;
    rc = fds_write(data_file, data_id, &data_to_store, sizeof(data_to_store));
    if (rc != FDS_SUCCESS) {
        NRF_LOG_INFO("FDS write failed");
    } else {
        NRF_LOG_INFO("FDS write OK");
    }

    int data_read = (int) fds_read(data_file, data_id);
    NRF_LOG_INFO("We read %d", data_read);

}

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
#include "nrf_atfifo.h"

#include "fds.h"
#include "flash_storage.h"

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


int main(void) {
    nrf_power_dcdcen_set(1);
    log_init();
    NRF_LOG_INFO("Start");

    //-------------------------------------------Setup-------------------------------------------

    flash_storage_init();

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
    // int data_to_store = 200201;
    // uint16_t data_file = 0x1234;
    // uint16_t data_id = 0x1425;
    // ret_code_t rc = fds_write(data_file, data_id, &data_to_store, sizeof(data_to_store));
    // if (rc != FDS_SUCCESS) {
    //     NRF_LOG_INFO("FDS write failed");
    // } else {
    //     NRF_LOG_INFO("FDS write OK");
    // }

    // int data_read = (int) fds_read(data_file, data_id);
    // NRF_LOG_INFO("We read %d", data_read);

    // int x3 = 1984;
    // flash_put("x3", &x3, sizeof(x3));

    // NRF_LOG_INFO("x1: %d", (int) flash_get("x1"));
    // NRF_LOG_INFO("x3: %d", (int) flash_get("x3"));

    //int x1 = 10;
    //define_flash_variable("x1", "int", 10); // TODO: implement this
    // This should somehow check if x1's value has been changed from 10, and if it has, change x2 to 10
    // actually, int x1 = (int) define_flash_variable("x1", "int", 10) might work too -- the 10 would just get 
    // ignored if x1 has a fresher value


    //NRF_LOG_INFO("1: %x", (uint16_t) "ab");

    //-----------------------------More Tests of Stuff------------------------------------------------------
    int x1 = (int) define_flash_variable(25, 0x1234, sizeof(int));
}

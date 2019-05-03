#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "nrf.h"
#include "fds.h"
#include "nrf_log.h"

#define DEFAULT_FILE_ID 0x1111
#define MAX_STR_LEN 256

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;

/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
    while (!m_fds_initialized)
    {
#ifdef SOFTDEVICE_PRESENT
        (void) sd_app_evt_wait();
#else
        __WFE();
#endif
    }
}

// #define FLASH_STORAGE_FLAG_FILE_ID 0x1010
// #define FLASH_STORAGE_FLAG_RECORD_KEY 0x9090

static void fds_evt_handler(fds_evt_t const * p_fds_evt) {
    switch (p_fds_evt->id) {
        case FDS_EVT_INIT:
            if (p_fds_evt->result == FDS_SUCCESS)
            {
                m_fds_initialized = true;
            }
            break;
        default:
            break;
    }
}

void flash_storage_init() {
    ret_code_t rc = fds_register(fds_evt_handler);
    if (rc != FDS_SUCCESS) {
        // Registering of the FDS event handler has failed.
        NRF_LOG_INFO("FDS register failed");
    } else {
        NRF_LOG_INFO("FDS register OK");
    }
    rc = fds_init();

    wait_for_fds_ready();

    if (rc != FDS_SUCCESS) {
        NRF_LOG_INFO("FDS init failed");
    } else {
        NRF_LOG_INFO("FDS init OK");
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

ret_code_t fds_update(uint16_t file_id, uint16_t record_key, void const *p_data, size_t data_size) {
    // Find the record description for the record we want to update
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;
    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    fds_record_find(file_id, record_key, &record_desc, &ftok);
    // Now record_desc should contain correct description

    fds_record_t record;
    record.file_id = file_id;
    record.key = record_key;
    record.data.p_data = p_data;
    record.data.length_words = (data_size + 3) / 4; // 4 bytes = 1 word; we take the ceiling of the # of words
    return fds_record_update(&record_desc, &record);
}

int define_flash_variable_int(int initial_value, uint16_t record_key) {
    int return_value;

    fds_flash_record_t flash_record;
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;

    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    if (fds_record_find(DEFAULT_FILE_ID, record_key, &record_desc, &ftok) == FDS_SUCCESS) {
        fds_record_open(&record_desc, &flash_record);
        return_value = *((int *)flash_record.p_data);
        fds_record_close(&record_desc);
    } else {
        fds_write(DEFAULT_FILE_ID, record_key, &initial_value, sizeof(int));
        return_value = initial_value;
    }
    return return_value;
}

float define_flash_variable_float(float initial_value, uint16_t record_key) {
    float return_value;

    fds_flash_record_t flash_record;
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;

    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    if (fds_record_find(DEFAULT_FILE_ID, record_key, &record_desc, &ftok) == FDS_SUCCESS) {
        fds_record_open(&record_desc, &flash_record);
        return_value = *((float *)flash_record.p_data);
        fds_record_close(&record_desc);
    } else {
        fds_write(DEFAULT_FILE_ID, record_key, &initial_value, sizeof(float));
        return_value = initial_value;
    }
    return return_value;
}

void define_flash_variable_array(const uint8_t *initial_value, uint8_t *dest, const size_t length, uint16_t record_key) {
    ret_code_t rc = FDS_SUCCESS;
    fds_flash_record_t flash_record;
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;

    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    rc = fds_record_find(DEFAULT_FILE_ID, record_key, &record_desc, &ftok);
    if (rc == FDS_SUCCESS) {
        rc = fds_record_open(&record_desc, &flash_record);
        APP_ERROR_CHECK(rc);
        memcpy(dest, flash_record.p_data, flash_record.p_header->length_words*4);
        rc = fds_record_close(&record_desc);
        APP_ERROR_CHECK(rc);
    } else {
        rc = fds_write(DEFAULT_FILE_ID, record_key, initial_value, length);
        APP_ERROR_CHECK(rc);
        memcpy(dest, initial_value, length);
    }
}

ret_code_t flash_update_int(const uint16_t record_key, const int value) {
    return fds_update(DEFAULT_FILE_ID, record_key, &value, sizeof(int));
}

ret_code_t flash_update_float(const uint16_t record_key, const float value) {
    return fds_update(DEFAULT_FILE_ID, record_key, &value, sizeof(float));
}

ret_code_t flash_update_array(const uint16_t record_key, const char* value, const size_t length) {
    return fds_update(DEFAULT_FILE_ID, record_key, &value, length);
}

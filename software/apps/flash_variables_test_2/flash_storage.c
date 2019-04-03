#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "nrf.h"
#include "fds.h"
#include "nrf_log.h"

#define DEFAULT_FILE_ID 0x1111
#define DEFAULT_RECORD_KEY 0x2222

#define FLASH_STORAGE_FLAG_FILE_ID 0x1010
#define FLASH_STORAGE_FLAG_RECORD_KEY 0x9090

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

static void set_flash_storage_flag() {
    int x = 1; // actual value doesn't matter -- just matters that record exists
    fds_write(FLASH_STORAGE_FLAG_FILE_ID, FLASH_STORAGE_FLAG_RECORD_KEY, &x, sizeof(x));
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
    if (rc != FDS_SUCCESS) {
        NRF_LOG_INFO("FDS init failed");
    } else {
        NRF_LOG_INFO("FDS init OK");
    }
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;
    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    rc = fds_record_find(FLASH_STORAGE_FLAG_FILE_ID, FLASH_STORAGE_FLAG_RECORD_KEY, &record_desc, &ftok);
    if (rc == FDS_ERR_NOT_FOUND) { // ie we've never used FDS before
        set_flash_storage_flag();
    }
} 

static ret_code_t fds_update(uint16_t file_id, uint16_t record_key, void const *p_data, size_t data_size) {
    // Find the record description for the record we want to update
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;
    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    // TODO: change this back to a while loop thing because we aren't going to vary record IDs anymore
    fds_record_find(file_id, record_key, &record_desc, &ftok);
    // Now record_desc should contain correct description
    
    fds_record_t record;
    record.file_id = file_id;
    record.key = record_key;
    record.data.p_data = p_data;
    record.data.length_words = (data_size + 3) / 4; // 4 bytes = 1 word; we take the ceiling of the # of words
    return fds_record_update(&record_desc, &record);
}

static uint32_t fds_read(uint16_t file_id, uint16_t record_key) {
// right now this assumes all data will be read back out as uint32's.
// Also only ever returns the first thing found with the matching file id and record key
    uint32_t return_value = 0;

    fds_flash_record_t flash_record;
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;
    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    // TODO: change this back to a while loop thing because we aren't going to vary record IDs anymore
    if (fds_record_find(file_id, record_key, &record_desc, &ftok) == FDS_SUCCESS) {
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

// static uint16_t get_record_key(const char* name) {
//     // Returns record_key if "name" is already present in array of flash variable names
//     // Else, returns 0
//     uint16_t record_key;
//     fds_flash_record_t flash_record;
//     fds_record_desc_t record_desc;
//     fds_find_token_t ftok;
//     memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
//     fds_record_find(NAME_MAP_FILE_ID, NAME_MAP_RECORD_KEY, &record_desc, &ftok);
//     fds_record_open(&record_desc, &flash_record);

//     bool found = false;
//     for (record_key = 1; record_key < NAME_MAP_SIZE; record_key++) { // skip 0th entry because 0x0000 can't be used as a record ID
//         if (strcmp(name, (*((char**) flash_record.p_data))[record_key] == 0)) {
//             found = true;
//             break;
//         }
//     }
//     fds_record_close(&record_desc);
//     if (found) {
//         return record_key;
//     }
//     return 0;
// }

char *make_int_string(const char *name, int data) {
    int length = snprintf(NULL, 0, "%x", data);
    char* int_str = malloc( length + 1 );
    snprintf(int_str, length + 1, "%x", data);
    printf("%s\n", int_str);
    free(int_str); 
}

ret_code_t flash_put(const char *name, void const *p_data, size_t data_size) {


    ret_code_t rc;
    fds_flash_record_t  flash_record;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok;
    memset(&ftok, 0x00, sizeof(fds_find_token_t));

    bool found = false;
    while (fds_record_find(DEFAULT_FILE_ID, DEFAULT_RECORD_KEY, &record_desc, &ftok) == FDS_SUCCESS && !found) {
        if (fds_record_open(&record_desc, &flash_record) == FDS_SUCCESS) {
            if (true) { // TODO: fix this
                found = true;
            }
        } else {
            NRF_LOG_INFO("FDS read open failed");
        }
        if (fds_record_close(&record_desc) != FDS_SUCCESS) {
             NRF_LOG_INFO("FDS read close failed");
        }
    }

    if (found) { // I.E. this is not a fresh variable and we're just updating
        return rc;
        // stick struct into flash using update()
        // This preserves an invariant that there's only ever one record at a time in flash with our "name"
    } else {
        // stick struct into flash using write()
        return rc;
    }
 
    //uint16_t record_key = get_record_key(name);
    //if (record_key > 0) { // Variable already exists
    //    rc = fds_update(DEFAULT_FILE_ID, record_key, p_data, data_size);
    //} else {
    //    rc = fds_write(DEFAULT_FILE_ID, record_key, p_data, data_size);
    //    flash_variable_names[next_record_key] = name; // I don't think this works ahahaha
    //    next_record_key++;
    //}
    return rc;
}

uint32_t flash_get(const char *name) {
    uint16_t record_key = get_record_key(name);
    if (record_key > 0) { // Variable already exists
        return fds_read(DEFAULT_FILE_ID, record_key);
    }
    NRF_LOG_INFO("You tried to read a variable that hasn't been stored in flash");
    return 0;
}
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "nrf.h"
#include "fds.h"
#include "nrf_log.h"

#define DEFAULT_FILE_ID 0x1111
// #define DEFAULT_RECORD_KEY 0x2222

// #define FLASH_STORAGE_FLAG_FILE_ID 0x1010
// #define FLASH_STORAGE_FLAG_RECORD_KEY 0x9090

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

// static void set_flash_storage_flag() {
//     int x = 1; // actual value doesn't matter -- just matters that record exists
//     fds_write(FLASH_STORAGE_FLAG_FILE_ID, FLASH_STORAGE_FLAG_RECORD_KEY, &x, sizeof(x));
// }

// static bool is_flash_storage_flag_set() {
//     fds_record_desc_t record_desc;
//     fds_find_token_t ftok;
//     memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
//     rc = fds_record_find(FLASH_STORAGE_FLAG_FILE_ID, FLASH_STORAGE_FLAG_RECORD_KEY, &record_desc, &ftok);
//     if (rc == FDS_ERR_NOT_FOUND) { // ie we've never used FDS before
//         return false;
//     }
//     return true;
// }

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
    // if (!is_flash_storage_flag_set()) {
    //     set_flash_storage_flag();
    // }

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

// static uint32_t fds_read(uint16_t file_id, uint16_t record_key) {
// // right now this assumes all data will be read back out as uint32's.
// // Also only ever returns the first thing found with the matching file id and record key
//     uint32_t return_value = 0;

//     fds_flash_record_t flash_record;
//     fds_record_desc_t record_desc;
//     fds_find_token_t ftok;
//     memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
//     // TODO: change this back to a while loop thing because we aren't going to vary record IDs anymore
//     if (fds_record_find(file_id, record_key, &record_desc, &ftok) == FDS_SUCCESS) {
//         if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS) {
//             NRF_LOG_INFO("FDS read open failed");
//         } else {
//         return_value = *((uint32_t *)flash_record.p_data);
//         }
//         if (fds_record_close(&record_desc) != FDS_SUCCESS) {
//              NRF_LOG_INFO("FDS read close failed");
//         }
//     }

//     return return_value;
// }

// static uint32_t fds_read(uint16_t file_id, uint16_t record_key) {
//     uint32_t return_value = 0;

//     fds_flash_record_t flash_record;
//     fds_record_desc_t record_desc;
//     fds_find_token_t ftok;
//     memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    
//     // TODO: change this back to a while loop thing because we aren't going to vary record IDs anymore
//     if (fds_record_find(file_id, record_key, &record_desc, &ftok) == FDS_SUCCESS) {
//         fds_record_open(&record_desc, &flash_record);
//         return_value = *((uint32_t *)flash_record.p_data);
//         fds_record_close(&record_desc);
//     }

//     return return_value;
// }

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

// ret_code_t flash_put(const char *name, void const *p_data, size_t data_size) {


//     ret_code_t rc;
//     fds_flash_record_t  flash_record;
//     fds_record_desc_t   record_desc;
//     fds_find_token_t    ftok;
//     memset(&ftok, 0x00, sizeof(fds_find_token_t));

//     bool found = false;
//     while (fds_record_find(DEFAULT_FILE_ID, DEFAULT_RECORD_KEY, &record_desc, &ftok) == FDS_SUCCESS && !found) {
//         if (fds_record_open(&record_desc, &flash_record) == FDS_SUCCESS) {
//             if (true) { // TODO: fix this
//                 found = true;
//             }
//         } else {
//             NRF_LOG_INFO("FDS read open failed");
//         }
//         if (fds_record_close(&record_desc) != FDS_SUCCESS) {
//              NRF_LOG_INFO("FDS read close failed");
//         }
//     }
//     }
// }

// uint32_t define_flash_variable(void *p_initial_value, uint16_t record_key, size_t size) {
//     uint32_t return_value = 0;

//     fds_flash_record_t flash_record;
//     fds_record_desc_t record_desc;
//     fds_find_token_t ftok;

//     memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
//     if (fds_record_find(DEFAULT_FILE_ID, record_key, &record_desc, &ftok) == FDS_SUCCESS) {
//         fds_record_open(&record_desc, &flash_record);
//         return_value = *((uint32_t *)flash_record.p_data);
//         fds_record_close(&record_desc);
//     } else {
//         fds_write(DEFAULT_FILE_ID, record_key, p_initial_value, size);
//         return_value = (uint32_t) *((uint32_t *) p_initial_value);
//     }
//     return return_value;
// }

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

void define_flash_variable_string(char *initial_value, char *dest, uint16_t record_key) {
    fds_flash_record_t flash_record;
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;

    memset(&ftok, 0x00, sizeof(fds_find_token_t)); // Zero the token
    if (fds_record_find(DEFAULT_FILE_ID, record_key, &record_desc, &ftok) == FDS_SUCCESS) {
        if (fds_record_open(&record_desc, &flash_record) == FDS_SUCCESS) {
            printf("Open OK\n");
        }
        strcpy(dest, *((char **) flash_record.p_data));
        printf("DEST: %s\n", dest);
        if (fds_record_close(&record_desc) == FDS_SUCCESS) {
            printf("Close OK\n");
        }
    } else {
        fds_write(DEFAULT_FILE_ID, record_key, &initial_value, sizeof(initial_value));
        strcpy(dest, initial_value);
    }
}


// uint32_t flash_get(const char *name) {
//     uint16_t record_key = get_record_key(name);
//     if (record_key > 0) { // Variable already exists
//         return fds_read(DEFAULT_FILE_ID, record_key);
//     }
//     NRF_LOG_INFO("You tried to read a variable that hasn't been stored in flash");
//     return 0;
// }


// ret_code_t flash_update(uint16_t record_key, void *p_value, size_t size) {
//     return fds_update(DEFAULT_FILE_ID, record_key, p_value, size);
// }

ret_code_t flash_update_int(uint16_t record_key, int value) {
    return fds_update(DEFAULT_FILE_ID, record_key, &value, sizeof(int));
}

ret_code_t flash_update_float(uint16_t record_key, int value) {
    return fds_update(DEFAULT_FILE_ID, record_key, &value, sizeof(float));
}

ret_code_t flash_update_string(uint16_t record_key, char* value) {
    return fds_update(DEFAULT_FILE_ID, record_key, value, sizeof(value));
}

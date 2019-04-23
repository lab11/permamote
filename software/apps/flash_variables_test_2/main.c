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

#define MAX_STR_LEN 256

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

    uint16_t s1_id = 0x1897;
    //uint16_t s2_id = 0x1896;
    char s1[MAX_STR_LEN] = "";
    //char s2[MAX_STR_LEN] = "";
    define_flash_variable_string("abcdefghijklmnop", s1, s1_id);
    //define_flash_variable_string("qwertyuiopasdf", s2, s2_id);
    nrf_delay_ms(1000);
    NRF_LOG_INFO("s1: %s\n", s1);
    //NRF_LOG_INFO("s2: %s\n", s2);
    flash_update_string(s1_id, "qwerty");

    // uint16_t int1_id = 0x1832;
    // int int1 = define_flash_variable_int(-1017, int1_id);
    // printf("int1: %d\n", int1);
    // flash_update_int(int1_id, 1206);

    // uint16_t float1_id = 0x1833;
    // float float1 = define_flash_variable_float(1011.12F, float1_id);
    // printf("float1: %f\n", float1);
    // flash_update_float(float1_id, 1206.2);
}
#pragma once

#define COAP_SERVER_HOSTNAME "coap-test.permamote.com"
#define NTP_SERVER_HOSTNAME "us.pool.ntp.org"
#define UPDATE_SERVER_HOSTNAME "dfu.permamote.com"
#define DNS_SERVER_ADDR "fdaa:bb:1::2"
#define GATEWAY_PARSE_ADDR "lab11.github.io/permamote/gateway/"
#define GATEWAY_DEVICE_TYPE "Permacam"

#define ID_LOCATOR 0x1000

#define DEFAULT_CHILD_TIMEOUT    2*60  /**< Thread child timeout [s]. */
#define DEFAULT_POLL_PERIOD      20000 /**< Thread Sleepy End Device polling period when Asleep. [ms] */
#define DFU_POLL_PERIOD          15
#define RECV_POLL_PERIOD         15    /**< Thread Sleepy End Device polling period when expecting response. [ms] */
#define NUM_SLAAC_ADDRESSES      6     /**< Number of SLAAC addresses. */

#define VOLTAGE_PERIOD      5
#define CAMERA_PERIOD       1
#define DISCOVER_PERIOD     15
#define SENSOR_PERIOD       APP_TIMER_TICKS(60*1000)
#define PIR_BACKOFF_PERIOD  APP_TIMER_TICKS(2*60*1000)
#define PIR_DELAY           APP_TIMER_TICKS(10*1000)
#define DFU_MONITOR_PERIOD  APP_TIMER_TICKS(5*1000)
#define COAP_TICK_TIME      APP_TIMER_TICKS(1*1000)
#define NTP_UPDATE_HOURS    1
#define NTP_UPDATE_MAX      5*1000
#define NTP_UPDATE_MIN      2*1000
#define DFU_CHECK_HOURS     24


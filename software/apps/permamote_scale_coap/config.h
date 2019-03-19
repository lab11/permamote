#define COAP_SERVER_HOSTNAME "coap.permamote.com"
#define NTP_SERVER_HOSTNAME "time.nist.gov"
#define DNS_SERVER_ADDR "fdaa:bb:1::2"
#define PARSE_ADDR "j2x.us/perm"

#define DEFAULT_CHILD_TIMEOUT    2*60  /**< Thread child timeout [s]. */
#define DEFAULT_POLL_PERIOD      60000 /**< Thread Sleepy End Device polling period when Asleep. [ms] */
#define RECV_POLL_PERIOD         10    /**< Thread Sleepy End Device polling period when expecting response. [ms] */
#define NUM_SLAAC_ADDRESSES      6     /**< Number of SLAAC addresses. */

#define VOLTAGE_PERIOD      2
#define TPH_PERIOD          5
#define WEIGHT_PERIOD       5
#define COLOR_PERIOD        10
#define DISCOVER_PERIOD     5
#define SENSOR_PERIOD       APP_TIMER_TICKS(60*1000)
#define RTC_UPDATE_FIRST    APP_TIMER_TICKS(4*1000)


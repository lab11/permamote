// Datasheet: https://ams.com/documents/20143/36005/TCS3472_DS000390_2-00.pdf
// Adapted from https://github.com/adafruit/Adafruit_TCS34725/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_twi_mngr.h"

#define TCS34725_ADDRESS          0x29

#define TCS34725_COMMAND_BIT      0x80 | (1 << 5)

#define TCS34725_ENABLE           0x00
#define TCS34725_ENABLE_AIEN      0x10    /* RGBC Interrupt Enable */
#define TCS34725_ENABLE_WEN       0x08    /* Wait enable - Writing 1 activates the wait timer */
#define TCS34725_ENABLE_AEN       0x02    /* RGBC Enable - Writing 1 actives the ADC, 0 disables it */
#define TCS34725_ENABLE_PON       0x01    /* Power on - Writing 1 activates the internal oscillator, 0 disables it */
#define TCS34725_ATIME            0x01    /* Integration time */
#define TCS34725_WTIME            0x03    /* Wait time (if TCS34725_ENABLE_WEN is asserted) */
#define TCS34725_WTIME_2_4MS      0xFF    /* WLONG0 = 2.4ms   WLONG1 = 0.029s */
#define TCS34725_WTIME_204MS      0xAB    /* WLONG0 = 204ms   WLONG1 = 2.45s  */
#define TCS34725_WTIME_614MS      0x00    /* WLONG0 = 614ms   WLONG1 = 7.4s   */
#define TCS34725_AILTL            0x04    /* Clear channel lower interrupt threshold */
#define TCS34725_AILTH            0x05
#define TCS34725_AIHTL            0x06    /* Clear channel upper interrupt threshold */
#define TCS34725_AIHTH            0x07
#define TCS34725_PERS             0x0C    /* Persistence register - basic SW filtering mechanism for interrupts */
#define TCS34725_CONFIG           0x0D
#define TCS34725_CONFIG_WLONG     0x02    /* Choose between short and long (12x) wait times via TCS34725_WTIME */
#define TCS34725_CONTROL          0x0F    /* Set the gain level for the sensor */
#define TCS34725_ID               0x12    /* 0x44 = TCS34721/TCS34725, 0x4D = TCS34723/TCS34727 */
#define TCS34725_STATUS           0x13
#define TCS34725_STATUS_AINT      0x10    /* RGBC clear channel interrupt */
#define TCS34725_STATUS_AVALID    0x01    /* Indicates that the RGBC channels have completed an integration cycle */
#define TCS34725_CDATAL           0x14    /* Clear channel data */
#define TCS34725_CDATAH           0x15
#define TCS34725_RDATAL           0x16    /* Red channel data */
#define TCS34725_RDATAH           0x17
#define TCS34725_GDATAL           0x18    /* Green channel data */
#define TCS34725_GDATAH           0x19
#define TCS34725_BDATAL           0x1A    /* Blue channel data */
#define TCS34725_BDATAH           0x1B

typedef enum
{
  TCS34725_INTEGRATIONTIME_2_4MS  = 0xFF,   /**<  2.4ms - 1 cycle    - Max Count: 1024  */
  TCS34725_INTEGRATIONTIME_24MS   = 0xF6,   /**<  24ms  - 10 cycles  - Max Count: 10240 */
  TCS34725_INTEGRATIONTIME_50MS   = 0xEB,   /**<  50ms  - 20 cycles  - Max Count: 20480 */
  TCS34725_INTEGRATIONTIME_101MS  = 0xD5,   /**<  101ms - 42 cycles  - Max Count: 43008 */
  TCS34725_INTEGRATIONTIME_154MS  = 0xC0,   /**<  154ms - 64 cycles  - Max Count: 65535 */
  TCS34725_INTEGRATIONTIME_700MS  = 0x00    /**<  700ms - 256 cycles - Max Count: 65535 */
} tcs34725IntegrationTime_t;

typedef enum
{
  TCS34725_GAIN_1X                = 0x00,   /**<  No gain  */
  TCS34725_GAIN_4X                = 0x01,   /**<  4x gain  */
  TCS34725_GAIN_16X               = 0x02,   /**<  16x gain */
  TCS34725_GAIN_60X               = 0x03    /**<  60x gain */
} tcs34725Gain_t;

typedef enum
{
  TCS34725_PERS_NONE       = 0x0,  /* Every RGBC cycle generates an interrupt                                */
  TCS34725_PERS_1_CYCLE    = 0x1,  /* 1 clear channel value outside threshold range generates an interrupt   */
  TCS34725_PERS_2_CYCLE    = 0x2,  /* 2 clear channel values outside threshold range generates an interrupt  */
  TCS34725_PERS_3_CYCLE    = 0x3,  /* 3 clear channel values outside threshold range generates an interrupt  */
  TCS34725_PERS_5_CYCLE    = 0x4,  /* 5 clear channel values outside threshold range generates an interrupt  */
  TCS34725_PERS_10_CYCLE   = 0x5,  /* 10 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_15_CYCLE   = 0x6,  /* 15 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_20_CYCLE   = 0x7,  /* 20 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_25_CYCLE   = 0x8,  /* 25 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_30_CYCLE   = 0x9,  /* 30 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_35_CYCLE   = 0xA,  /* 35 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_40_CYCLE   = 0xB,  /* 40 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_45_CYCLE   = 0xC,  /* 45 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_50_CYCLE   = 0xD,  /* 50 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_55_CYCLE   = 0xE,  /* 55 clear channel values outside threshold range generates an interrupt */
  TCS34725_PERS_60_CYCLE   = 0xF   /* 60 clear channel values outside threshold range generates an interrupt */
} tcs34725Pers_t;

typedef struct {
  tcs34725Gain_t            gain;
  tcs34725IntegrationTime_t int_time;
} tcs34725_config_t;

// struct for auto-gain configuration
typedef struct {
  tcs34725_config_t config;
  uint16_t mincnt;
  uint16_t maxcnt;
} tcs34725_agc_t;

typedef void tcs34725_read_callback_t(uint16_t r, uint16_t g, uint16_t b, uint16_t c);

void  tcs34725_init(const nrf_twi_mngr_t* instance);
void  tcs34725_on(void);
void  tcs34725_off(void);
void  tcs34725_config_agc(void);
void  tcs34725_config(tcs34725_config_t config);
void  tcs34725_enable(void);
void  tcs34725_disable(void);
void  tcs34725_read_channels_agc(tcs34725_read_callback_t* cb);
void  tcs34725_read_channels(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* c);
void  tcs34725_ir_compensate(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* c);
float tcs34725_calculate_ct(uint16_t r, uint16_t b);
float tcs34725_calculate_cct(uint16_t r, uint16_t g, uint16_t b);
//float tcs34725_calculate_lux(uint16_t r, uint16_t g, uint16_t b);

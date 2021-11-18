#include "nrf_twi_mngr.h"

#define GRIDEYE_I2C_ADDR_0   0x68
#define GRIDEYE_I2C_ADDR_1   0x69
#define GRIDEYE_I2C_ADDR     GRIDEYE_I2C_ADDR_1

#define GRIDEYE_PCLT_ADDR    0x00
#define GRIDEYE_RST_ADDR     0x01
#define GRIDEYE_FPSC_ADDR    0x02
#define GRIDEYE_INTC_ADDR    0x03
#define GRIDEYE_STAT_ADDR    0x04
#define GRIDEYE_SCLR_ADDR    0x05
#define GRIDEYE_AVE_ADDR     0x07
#define GRIDEYE_INTHL_ADDR   0x08
#define GRIDEYE_INTHH_ADDR   0x09
#define GRIDEYE_INTLH_ADDR   0x0A
#define GRIDEYE_INTLL_ADDR   0x0B
#define GRIDEYE_INTLL_ADDR   0x0B

#define GRIDEYE_NUM_PIXELS   64
#define GRIDEYE_T01L_ADDR    0x80
#define GRIDEYE_T01H_ADDR    0x81

typedef enum {
  GRIDEYE_PCLT_NORMAL = 0x00,
  GRIDEYE_PCLT_SLEEP  = 0x10,
  GRIDEYE_PCLT_STDBY_60  = 0x20,
  GRIDEYE_PCLT_STDBY_10  = 0x21,
} grideye_pclt_t;

typedef enum {
  GRIDEYE_FLAG_RST = 0x30,
  GRIDEYE_INIT_RST = 0x3F,
} grideye_rst_t;

typedef enum {
  GRIDEYE_FPSC_1FPS  = 0x01,
  GRIDEYE_FPSC_10FPS = 0x00,
} grideye_fpsc_t;

// initialize driver with twi instance
void grideye_init(const nrf_twi_mngr_t * twi_instance);

// reset and configure pclt and fpsc registers
void grideye_config(grideye_pclt_t pclt, grideye_fpsc_t fpsc);

void grideye_read_pixels(uint16_t * pixels);

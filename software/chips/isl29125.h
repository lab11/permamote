// Datasheet: https://www.intersil.com/content/dam/Intersil/documents/isl2/isl29125.pdf

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_twi_mngr.h"

#define ISL29125_ADDR   0x44
#define ISL29125_ID_RST 0x00
#define ISL29125_CFG1   0x01
#define ISL29125_CFG2   0x02
#define ISL29125_CFG3   0x03
#define ISL29125_STATUS 0x08
#define ISL29125_G_LOW  0x09
#define ISL29125_G_HIGH 0x0A
#define ISL29125_R_LOW  0x0B
#define ISL29125_R_HIGH 0x0C
#define ISL29125_B_LOW  0x0D
#define ISL29125_B_HIGH 0x0E

typedef void isl29125_color_read_callback_t(uint16_t red, uint16_t green, uint16_t blue);
typedef void isl29125_interrupt_callback_t(void);

typedef enum {
  isl29125_mode_power_down     = 0,
  isl29125_mode_green          = 1,
  isl29125_mode_red            = 2,
  isl29125_mode_blue           = 3,
  isl29125_mode_standby        = 4,
  isl29125_mode_green_red_blue = 5,
  isl29125_mode_green_red      = 6,
  isl29125_mode_green_blue     = 7
} isl29125_mode_t;

typedef enum {
  isl29125_int_none= 0,
  isl29125_int_green,
  isl29125_int_red,
  isl29125_int_blue
} isl29125_interrupt_t;

typedef enum {
  isl29125_int_p_1= 0,
  isl29125_int_p_2,
  isl29125_int_p_4,
  isl29125_int_p_8,
} isl29125_int_persist_t;

typedef struct {
  isl29125_mode_t mode;
  bool range;           // 0 = range of 375 lux, 1 = range of 10000 lux
  bool resolution;      // 0 = 16 bit resolution, 1 = 12 bit resolution
  bool sync_int;        // 0 = adc start at i2c write 0x01, 1 = adc start at rising int
} isl29125_config_t;

typedef struct {
  isl29125_interrupt_t mode;
  isl29125_int_persist_t persist;
  bool int_conversion_done;
  isl29125_interrupt_callback_t* callback;
} isl29125_int_config_t;

void isl29125_init(const nrf_twi_mngr_t*            instance,
                   isl29125_config_t                config,
                   isl29125_int_config_t            int_config,
                   isl29125_color_read_callback_t*  read_callback,
                   isl29125_interrupt_callback_t*   int_callback);
void isl29125_configure(isl29125_config_t config);
void isl29125_interrupt_configure(isl29125_int_config_t config);
uint8_t isl29125_read_status();
void isl29125_schedule_color_read(void);
void isl29125_interrupt_enable(void);
void isl29125_interrupt_disable(void);
void isl29125_power_down(void);
void isl29125_power_on(void);

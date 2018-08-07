// Datasheet: http://www.st.com/content/ccc/resource/technical/document/datasheet/group3/30/3a/4e/6b/68/16/4a/35/DM00323179/files/DM00323179.pdf/jcr:content/translations/en.DM00323179.pdf
#pragma once

#include "nrf_spi_mngr.h"

#define G                     9.8
#define LIS2DW12_CTRL1        0x20
#define LIS2DW12_CTRL2        0x21
#define LIS2DW12_CTRL3        0x22
#define LIS2DW12_CTRL4_INT1   0x23
#define LIS2DW12_CTRL5_INT2   0x24
#define LIS2DW12_CTRL6        0x25
#define LIS2DW12_CTRL7        0x3F
#define LIS2DW12_STATUS       0x27
#define LIS2DW12_OUT_X_L      0x28
#define LIS2DW12_FIFO_CTRL    0x2E
#define LIS2DW12_WAKE_UP_THS  0x34
#define LIS2DW12_WAKE_UP_DUR  0x35


typedef enum {
  lis2dw12_odr_power_down = 0,
  lis2dw12_odr_12_5_1_6,  // High power/Low power 12.5/1.6 Hz
  lis2dw12_odr_12_5,      // High power/Low power 12.5 Hz
  lis2dw12_odr_25,        // High power/Low power 25 Hz
  lis2dw12_odr_50,        // High power/Low power 50 Hz
  lis2dw12_odr_100,       // High power/Low power 100 Hz
  lis2dw12_odr_200,       // High power/Low power 200 Hz
  lis2dw12_odr_400,       // High power/Low power 400/200 Hz
  lis2dw12_odr_800,       // High power/Low power 800/200 Hz
  lis2dw12_odr_1600,      // High power/Low power 1600/200 Hz
} lis2dw12_odr_t;

typedef enum {
  lis2dw12_low_power = 0,
  lis2dw12_high_performance,
  lis2dw12_on_demand,
} lis2dw12_mode_t;

typedef enum {
  lis2dw12_lp_1 = 0,
  lis2dw12_lp_2,
  lis2dw12_lp_3,
  lis2dw12_lp_4,
} lis2dw12_lp_mode_t;

typedef enum {
  lis2dw12_bw_odr_2 = 0,
  lis2dw12_bw_odr_4,
  lis2dw12_bw_odr_10,
  lis2dw12_bw_odr_20,
} lis2dw12_bandwidth_t;

typedef enum {
  lis2dw12_fs_2g = 0,
  lis2dw12_fs_4g,
  lis2dw12_fs_8g,
  lis2dw12_fs_16g,
} lis2dw12_full_scale_t;

typedef enum {
  lis2dw12_fifo_bypass= 0,
  lis2dw12_fifo_stop = 1,
  lis2dw12_fifo_cont_to_fifo = 3,
  lis2dw12_fifo_byp_to_cont = 4,
  lis2dw12_fifo_continuous = 6,
} lis2dw12_fifo_mode_t;

typedef struct {
  lis2dw12_odr_t odr;
  lis2dw12_mode_t mode;
  lis2dw12_lp_mode_t lp_mode;
  bool cs_nopull;
  bool bdu;
  bool auto_increment;
  bool i2c_disable;
  bool sim;
  //bool int_open_drain;
  //bool int_latch;
  bool int_active_low;
  bool on_demand;
  lis2dw12_bandwidth_t bandwidth;
  lis2dw12_full_scale_t fs;
  bool high_pass;
  bool low_noise;
} lis2dw12_config_t;

typedef struct {
  bool int1_6d;
  bool int1_sngl_tap;
  bool int1_wakeup;
  bool int1_free_fall;
  bool int1_dbl_tap;
  bool int1_fifo_full;
  bool int1_fifo_thresh;
  bool int1_data_ready;
  bool int2_sleep_state;
  bool int2_sleep_change;
  bool int2_boot;
  bool int2_temp_ready;
  bool int2_fifo_over;
  bool int2_fifo_full;
  bool int2_fifo_thresh;
  bool int2_data_ready;
} lis2dw12_int_config_t;

typedef struct {
  lis2dw12_fifo_mode_t mode;
  uint8_t thresh; // 0 - 32
} lis2dw12_fifo_config_t;

typedef struct {
  bool sleep_enable;
  uint8_t threshold;
  uint8_t wake_duration;
  uint8_t sleep_duration;
} lis2dw12_wakeup_config_t;

typedef struct {
  bool fifo_thresh;
  bool wakeup;
  bool sleep;
  bool dbl_tap;
  bool sngl_tap;
  bool _6D;
  bool free_fall;
  bool data_ready;
} lis2dw12_status_t;

typedef void (*lis2dw12_read_full_fifo_callback_t) (void);

void  lis2dw12_init(const nrf_spi_mngr_t* instance);
void  lis2dw12_config(lis2dw12_config_t config);
void  lis2dw12_interrupt_config(lis2dw12_int_config_t config);
void  lis2dw12_interrupt_enable(bool enable);
void  lis2dw12_fifo_config(lis2dw12_fifo_config_t config);
void  lis2dw12_read_full_fifo(int16_t* x, int16_t* y, int16_t* z, lis2dw12_read_full_fifo_callback_t callback);
void  lis2dw12_wakeup_config(lis2dw12_wakeup_config_t wake_config);
lis2dw12_status_t lis2dw12_read_status(void);
void  lis2dw12_read_reg(uint8_t reg, uint8_t* read_buf, size_t len);
void  lis2dw12_write_reg(uint8_t reg, uint8_t* write_buf, size_t len);
void  lis2dw12_reset();
void  lis2dw12_off();
void  lis2dw12_on();

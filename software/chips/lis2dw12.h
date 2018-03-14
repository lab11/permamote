// Datasheet: http://www.st.com/content/ccc/resource/technical/document/datasheet/group3/30/3a/4e/6b/68/16/4a/35/DM00323179/files/DM00323179.pdf/jcr:content/translations/en.DM00323179.pdf

#include "nrf_drv_spi.h"

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
  bool cs_pullup;
  bool bdu;
  bool auto_increment;
  bool i2c_disable;
  bool sim;
  //bool int_open_drain;
  //bool int_latch;
  //bool int_active_high;
  bool on_demand;
  lis2dw12_bandwidth_t bandwidth;
  lis2dw12_full_scale_t fs;
  bool high_pass;
  bool low_noise;
} lis2dw12_config_t;

typedef struct {
  bool int1_6d;
  bool int1_single_tap;
  bool int1_wakeup;
  bool int1_free_fall;
  bool int1_double_tap;
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
  lis2dw12_fifo_mode_t config;
  uint8_t fifo_thresh; // 0 - 32
} lis2dw12_fifo_config_t;

typedef struct {
  bool sleep_enable;
  uint8_t threshold;
} lis2dw12_wakeup_config_t;

void  lis2dw12_init(nrf_drv_spi_t* instance, uint8_t cs);
void  lis2dw12_config(lis2dw12_config_t ctl_config);
void  lis2dw12_interrupt(lis2dw12_int_config_t int_config);
void  lis2dw12_fifo_config(lis2dw12_fifo_config_t config);
void  lis2dw12_off();
void  lis2dw12_on();

#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "app_error.h"

#include "lis2dw12.h"
#include "permamote.h"

static bool power_on = false;
static nrf_drv_spi_t* spi_instance;
static lis2dw12_config_t ctl_config;
//static lis2dw12_int_config_t int_config;

void  lis2dw12_init(nrf_drv_spi_t* instance, uint8_t cs) {
  spi_instance = instance;
  cs = cs;
}

void  lis2dw12_config(lis2dw12_config_t ctl_config) {
  ret_code_t err_code = 0;
  ctl_config = ctl_config;
  //int_config = int_config;

  if (ctl_config.odr > lis2dw12_odr_power_down) power_on = true;

  uint8_t ctl[7] = {0};
  ctl[0] = 0x20;
  ctl[1] = ctl_config.odr << 4 | (ctl_config.mode & 0x3) << 2 |
           (ctl_config.lp_mode & 0x3);
  ctl[2] = 0x8 | ctl_config.cs_pullup << 4 | ctl_config.bdu << 3 |
           ctl_config.auto_increment << 2 | ctl_config.i2c_disable << 1 |
           ctl_config.sim;
  ctl[3] = ctl_config.on_demand << 1;
  ctl[4] = 0x0;
  ctl[5] = 0x0;
  ctl[6] = ctl_config.bandwidth << 6 | ctl_config.fs << 4 |
           ctl_config.high_pass << 3 | ctl_config.low_noise << 2;
  nrf_gpio_pin_clear(LI2D_CS);
  err_code = nrf_drv_spi_transfer(spi_instance, ctl, 7, NULL, 0);
  nrf_gpio_pin_set(LI2D_CS);
  APP_ERROR_CHECK(err_code);
}

void  lis2dw12_interrupt(lis2dw12_int_config_t int_config) {
  ret_code_t err_code = 0;
  uint8_t ctl[3] = {0};
  ctl[0] = 0x23;
  ctl[1] = int_config.int1_6d << 7 | int_config.int1_single_tap << 6 |
           int_config.int1_wakeup << 5 | int_config.int1_free_fall << 4 |
           int_config.int1_double_tap << 3 | int_config.int1_fifo_full << 2 |
           int_config.int1_fifo_thresh << 1 | int_config.int1_data_ready;
  ctl[2] = int_config.int2_sleep_state << 7 | int_config.int2_sleep_change << 6 |
           int_config.int2_boot << 5 | int_config.int2_data_ready << 4 |
           int_config.int2_fifo_over << 3 | int_config.int2_fifo_full << 2 |
           int_config.int2_fifo_thresh << 1 | int_config.int2_data_ready;

  nrf_gpio_pin_clear(LI2D_CS);
  err_code = nrf_drv_spi_transfer(spi_instance, ctl, 3, NULL, 0);
  nrf_gpio_pin_set(LI2D_CS);
  APP_ERROR_CHECK(err_code);
}

void  lis2dw12_off() {
  ret_code_t err_code = 0;
  if (!power_on) return;

  uint8_t ctl[2] = {0};
  ctl[0] = 0x20;
  lis2dw12_odr_t odr_power_down = lis2dw12_odr_power_down;
  ctl[1] = odr_power_down << 4 | (ctl_config.mode & 0x3) << 2 |
          (ctl_config.lp_mode & 0x3);

  nrf_gpio_pin_clear(LI2D_CS);
  err_code = nrf_drv_spi_transfer(spi_instance, ctl, 2, NULL, 0);
  nrf_gpio_pin_set(LI2D_CS);
  APP_ERROR_CHECK(err_code);
}

void  lis2dw12_on() {
  ret_code_t err_code = 0;
  if (power_on) return;

  uint8_t ctl[2] = {0};
  ctl[0] = 0x20;
  ctl[1] = ctl_config.odr << 4 | (ctl_config.mode & 0x3) << 2 |
           (ctl_config.lp_mode & 0x3);

  nrf_gpio_pin_clear(LI2D_CS);
  err_code = nrf_drv_spi_transfer(spi_instance, ctl, 2, NULL, 0);
  nrf_gpio_pin_set(LI2D_CS);
  APP_ERROR_CHECK(err_code);
}

void lis2dw12_wakeup_config(lis2dw12_wakeup_config_t wake_config) {
  ret_code_t err_code = 0;
  uint8_t ctl[2] = {0};

  ctl[0] = 0x34;
  ctl[1] = wake_config.sleep_enable << 6 | (wake_config.threshold & 0x3f);

  nrf_gpio_pin_clear(LI2D_CS);
  err_code = nrf_drv_spi_transfer(spi_instance, ctl, 2, NULL, 0);
  nrf_gpio_pin_set(LI2D_CS);
  APP_ERROR_CHECK(err_code);
}

//void  lis2dw12_fifo_config();

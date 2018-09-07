#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "app_error.h"

#include "ab1815.h"

static const nrf_drv_spi_t* spi_instance;
static ab1815_control_t ctrl_config;
static nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;

void ab1815_init(const nrf_drv_spi_t* instance) {
  spi_instance = instance;

  spi_config.sck_pin    = SPI_SCLK;
  spi_config.miso_pin   = SPI_MISO;
  spi_config.mosi_pin   = SPI_MOSI;
  spi_config.ss_pin     = RTC_CS;
  spi_config.frequency  = NRF_DRV_SPI_FREQ_2M;
  spi_config.mode       = NRF_DRV_SPI_MODE_0;
  spi_config.bit_order  = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
}

void  ab1815_read_reg(uint8_t reg, uint8_t* read_buf, size_t len){
  if (len > 256) return;
  uint8_t readreg = reg;
  uint8_t buf[257];

  nrf_drv_spi_init(spi_instance, &spi_config, NULL, NULL);
  nrf_drv_spi_transfer(spi_instance, &readreg, 1, buf, len+1);
  nrf_drv_spi_uninit(spi_instance);

  memcpy(read_buf, buf+1, len);
}

void ab1815_write_reg(uint8_t reg, uint8_t* write_buf, size_t len){
  if (len > 256) return;
  uint8_t buf[257];
  buf[0] = 0x80 | reg;
  memcpy(buf+1, write_buf, len);

  nrf_drv_spi_init(spi_instance, &spi_config, NULL, NULL);
  nrf_drv_spi_transfer(spi_instance, buf, len+1, NULL, 0);
  nrf_drv_spi_uninit(spi_instance);
}

void ab1815_set_config(ab1815_control_t config) {
  uint8_t write;
  write =  config.stop << 7 | config.hour_12 << 6 | config.OUTB << 5 |
            config.OUT << 4 | config.rst_pol << 3 | config.auto_rst << 2 |
            0x2 | config.write_rtc;
  ctrl_config = config;

  //nrf_spi_mngr_transfer_t const config_transfer[] = {
  //  NRF_SPI_MNGR_TRANSFER(write, 2, NULL, 0),
  //};

  //int error = nrf_spi_mngr_perform(spi_instance, &spi_config, config_transfer, 1, NULL);
  //APP_ERROR_CHECK(error);
  ab1815_write_reg(AB1815_CONTROL1, &write, 1);
}

void ab1815_get_config(ab1815_control_t* config) {
  uint8_t read;

  //nrf_spi_mngr_transfer_t const config_transfer[] = {
  //  NRF_SPI_MNGR_TRANSFER(write, 1, read, 2),
  //};

  //int error = nrf_spi_mngr_perform(spi_instance, &spi_config, config_transfer, 1, NULL);
  //APP_ERROR_CHECK(error);

  //printf("%x\n", read[1]);

  ab1815_read_reg(AB1815_CONTROL1, &read, 1);

  config->stop      = (read & 0x80) >> 7;
  config->hour_12   = (read & 0x40) >> 6;
  config->OUTB      = (read & 0x20) >> 5;
  config->OUT       = (read & 0x10) >> 4;
  config->rst_pol   = (read & 0x08) >> 3;
  config->auto_rst  = (read & 0x04) >> 2;
  config->write_rtc = read & 0x01;
}

void ab1815_interrupt_config(ab1815_int_config_t config) {
  return;
}

inline uint8_t get_tens(uint8_t x) {
  return (x / 10) % 10;
}

inline uint8_t get_ones(uint8_t x) {
  return x % 10;
}

void ab1815_set_time(ab1815_time_t time) {
  uint8_t write[8];

  // Ensure rtc write bit is enabled
  if (ctrl_config.write_rtc != 1) {
    ctrl_config.write_rtc = 1;
    ab1815_set_config(ctrl_config);
  }

  APP_ERROR_CHECK_BOOL(time.hundredths < 100 && time.hundredths >= 0);
  APP_ERROR_CHECK_BOOL(time.seconds < 60 && time.seconds>= 0);
  APP_ERROR_CHECK_BOOL(time.minutes < 60 && time.minutes >= 0);
  APP_ERROR_CHECK_BOOL(time.hours < 24 && time.hours >= 0);
  APP_ERROR_CHECK_BOOL(time.date <= 31 && time.date >= 1);
  APP_ERROR_CHECK_BOOL(time.months <= 12 && time.date >= 1);
  APP_ERROR_CHECK_BOOL(time.years < 100 && time.date >= 0);
  APP_ERROR_CHECK_BOOL(time.weekday < 7 && time.weekday >= 0);

  write[0] = (get_tens(time.hundredths) & 0xF) << 4  | (get_ones(time.hundredths) & 0xF);
  write[1] = (get_tens(time.seconds) & 0x7) << 4    | (get_ones(time.seconds) & 0xF);
  write[2] = (get_tens(time.minutes) & 0x7) << 4    | (get_ones(time.minutes) & 0xF);
  write[3] = (get_tens(time.hours) & 0x3) << 4      | (get_ones(time.hours) & 0xF);
  write[4] = (get_tens(time.date) & 0x3) << 4       | (get_ones(time.date) & 0xF);
  write[5] = (get_tens(time.months) & 0x1) << 4     | (get_ones(time.months) & 0xF);
  write[6] = (get_tens(time.years) & 0xF) << 4      | (get_ones(time.years) & 0xF);
  write[7] = time.weekday & 0x7;

  ab1815_write_reg(AB1815_HUND, write, 8);

  //nrf_spi_mngr_transfer_t const write_time_transfer[] = {
  //  NRF_SPI_MNGR_TRANSFER(write, 9, NULL, 0),
  //};

  //int error = nrf_spi_mngr_perform(spi_instance, &spi_config, write_time_transfer, 1, NULL);
  //APP_ERROR_CHECK(error);
}

void ab1815_get_time(ab1815_time_t* time) {
  uint8_t read[10];

  ab1815_read_reg(AB1815_HUND, read, 8);

  //nrf_spi_mngr_transfer_t const config_transfer[] = {
  //  NRF_SPI_MNGR_TRANSFER(write, 1, read, 9),
  //};

  //int error = nrf_spi_mngr_perform(spi_instance, &spi_config, config_transfer, 1, NULL);

  time->hundredths = 10 * ((read[0] & 0xF0) >> 4) + (read[0] & 0xF);
  time->seconds   = 10 * ((read[1] & 0x70) >> 4) + (read[1] & 0xF);
  time->minutes   = 10 * ((read[2] & 0x70) >> 4) + (read[2] & 0xF);
  // TODO handle 12 hour format
  time->hours     = 10 * ((read[3] & 0x30) >> 4) + (read[3] & 0xF);
  time->date      = 10 * ((read[4] & 0x30) >> 4) + (read[4] & 0xF);
  time->months    = 10 * ((read[5] & 0x10) >> 4) + (read[5] & 0xF);
  time->years     = 10 * ((read[6] & 0xF0) >> 4) + (read[6] & 0xF);
  time->weekday   = read[7] & 0x7;
}

struct timeval ab1815_get_time_unix(void) {
  ab1815_time_t time;
  ab1815_get_time(&time);
  return ab1815_to_unix(time);
}

ab1815_time_t unix_to_ab1815(struct timeval tv) {
  ab1815_time_t time;
  struct tm * t;
  t = gmtime((time_t*)&(tv.tv_sec));
  time.hundredths = tv.tv_usec / 10000;
  time.seconds  = t->tm_sec;
  time.minutes  = t->tm_min;
  time.hours    = t->tm_hour;
  time.date     = t->tm_mday;
  time.months   = t->tm_mon + 1;
  time.years    = t->tm_year - 100;
  time.weekday  = t->tm_wday;
  return time;
}

struct timeval ab1815_to_unix(ab1815_time_t time) {
  struct timeval unix_time;
  struct tm t;
  t.tm_sec = time.seconds;
  t.tm_min = time.minutes;
  t.tm_hour = time.hours;
  t.tm_mday = time.date;
  t.tm_mon = time.months - 1;
  t.tm_year = time.years + 100;
  t.tm_wday = time.weekday;
  unix_time.tv_sec = mktime(&t);
  unix_time.tv_usec = time.hundredths * 10000;

  return unix_time;
}


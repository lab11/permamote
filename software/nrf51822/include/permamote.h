#ifndef PERMAMOTE_H
#define PERMAMOTE_H

#ifndef DEVICE_NAME
#define DEVICE_NAME "PERMAMOTE"
#endif /*DEVICE_NAME*/

#include <stdint.h>

//#define PLATFORM_ID_BYTE 0x22

// Address is written here in flash if the ID command is used
//#define ADDRESS_FLASH_LOCATION 0x0003fff8

// I2C through headers
#define I2C_SCL     10
#define I2C_SDA     8

// SPI through headers
#define SPI_SCLK    13
#define SPI_MISO    11
#define SPI_MOSI    12

// Light Sensor
#define MAX44009_EN  18
#define MAX44009_INT 28

// Color Sensor
#define ISL29125_EN  17
#define ISL29125_INT 29

// Pressure sensor
#define MS5637_EN   20

//Humidity Sensor
#define SI7021_EN   15

// Accelerometer
#define L12D_CS     14
#define L12D_INT1   6
#define L12D_INT2   7
#define L12D_EN     22

//PIR
#define PIR_EN      19
#define PIR_OUT     30

//RTC
#define RTC_CS      22
#define RTC_INT1    25
#define RTC_INT2    24
#define RTC_INT3    23
#define RTC_WDI     21

//ICOUNT
#define SWTICH      4

//BATTERIES
#define VSOL        3 
#define VPRIMARY    2 
#define VSECONDARY  1 

#endif

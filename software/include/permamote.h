#ifndef PERMAMOTE_H
#define PERMAMOTE_H

#ifndef DEVICE_NAME
#define DEVICE_NAME "Permamote"
#endif /*DEVICE_NAME*/

#include <stdint.h>

//#define PLATFORM_ID_BYTE 0x22

// DEFINES FOR NRF52840
#if defined(NRF52840_XXAA) || defined(NRF52)

// UART
#define UART_RX 26
#define UART_TX 27

// I2C
#define I2C_SCL     NRF_GPIO_PIN_MAP(0,14)
#define I2C_SDA     NRF_GPIO_PIN_MAP(0,15)

// SPI
#define SPI_SCLK    NRF_GPIO_PIN_MAP(0,12)
#define SPI_MISO    NRF_GPIO_PIN_MAP(1,9)
#define SPI_MOSI    NRF_GPIO_PIN_MAP(0,8)

// Light Sensor
#define MAX44009_EN  NRF_GPIO_PIN_MAP(0, 24)
#define MAX44009_INT NRF_GPIO_PIN_MAP(1,0)

// Color Sensor
#define ISL29125_EN  NRF_GPIO_PIN_MAP(0,25)
#define ISL29125_INT NRF_GPIO_PIN_MAP(0,23)
#define TCS34725_EN  ISL29125_EN
#define TCS34725_INT ISL29125_INT

// Pressure sensor
#define MS5637_EN   NRF_GPIO_PIN_MAP(1,11)

//Humidity Sensor
#define SI7021_EN   NRF_GPIO_PIN_MAP(1,10)

// Accelerometer
#define LI2D_CS     NRF_GPIO_PIN_MAP(0,21)
#define LI2D_INT1   NRF_GPIO_PIN_MAP(0,20)
#define LI2D_INT2   NRF_GPIO_PIN_MAP(0,22)

//PIR
#define PIR_EN      NRF_GPIO_PIN_MAP(0,17)
#define PIR_OUT     NRF_GPIO_PIN_MAP(0,16)

//RTC
#define RTC_CS      NRF_GPIO_PIN_MAP(1,12)
#define RTC_INT1    NRF_GPIO_PIN_MAP(1,13)
#define RTC_INT2    NRF_GPIO_PIN_MAP(1,14)
#define RTC_INT3    NRF_GPIO_PIN_MAP(1,15)
#define RTC_WDI     NRF_GPIO_PIN_MAP(0,2)

//ICOUNT
#define SWITCH      NRF_GPIO_PIN_MAP(0,13)
#define LBST        NRF_GPIO_PIN_MAP(0,19)

//BATTERIES
#define VSOL        NRF_GPIO_PIN_MAP(0,30)
#define VPRIMARY    NRF_GPIO_PIN_MAP(0,29)
#define VSECONDARY  NRF_GPIO_PIN_MAP(0,31)
#define VBAT_OK     NRF_GPIO_PIN_MAP(1,6)

// DEFINES FOR NRF51822
#elif defined(NRF51822)

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
#define LI2D_CS     14
#define LI2D_INT1   6
#define LI2D_INT2   7

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
#define SWITCH      4

//BATTERIES
#define VSOL        3
#define VPRIMARY    2
#define VSECONDARY  1

#endif //NRF51822
#endif

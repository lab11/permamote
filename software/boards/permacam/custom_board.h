// Pin definitions for nRF52 development kit (PCA10040)

#pragma once

#include "nrf_gpio.h"

// LED
#define LEDS_NUMBER 3
#define LEDS_ACTIVE_STATE 0
#define LED_1 NRF_GPIO_PIN_MAP(0,27)
#define LED_2 NRF_GPIO_PIN_MAP(0,4)
#define LED_3 NRF_GPIO_PIN_MAP(0,5)
#define LEDS_INV_MASK LEDS_MASK
#define LEDS_LIST { LED_1, LED_2, LED_3 }
#define BSP_LED_0 LED_1
#define BSP_LED_1 LED_2
#define BSP_LED_2 LED_3

// BUTTONS
#define BUTTONS_NUMBER 0
#define BUTTON_PULL 0

// I2C
#define I2C_SCL     NRF_GPIO_PIN_MAP(0,14)
#define I2C_SDA     NRF_GPIO_PIN_MAP(0,13)

// SPI
#define SPI_SCLK    NRF_GPIO_PIN_MAP(0,7)
#define SPI_MISO    NRF_GPIO_PIN_MAP(0,8)
#define SPI_MOSI    NRF_GPIO_PIN_MAP(0,6)

// Camera
#define HM01B0_EN       NRF_GPIO_PIN_MAP(0,16)
#define HM01B0_ENn      NRF_GPIO_PIN_MAP(0,15)
#define HM01B0_PCLKO    NRF_GPIO_PIN_MAP(0,20)
#define HM01B0_MCLK     NRF_GPIO_PIN_MAP(0,22)
#define HM01B0_FVLD     NRF_GPIO_PIN_MAP(0,24)
#define HM01B0_LVLD     NRF_GPIO_PIN_MAP(1,0)
#define HM01B0_CAM_D0   NRF_GPIO_PIN_MAP(0,17)
#define HM01B0_CAM_TRIG NRF_GPIO_PIN_MAP(0,23)
#define HM01B0_CAM_INT  NRF_GPIO_PIN_MAP(0,25)

// Light Sensor
#define MAX44009_EN  NRF_GPIO_PIN_MAP(0,28)
#define MAX44009_INT NRF_GPIO_PIN_MAP(0,3)

// Color Sensor
//#define ISL29125_EN  NRF_GPIO_PIN_MAP(0,25)
//#define ISL29125_INT NRF_GPIO_PIN_MAP(0,23)
//#define TCS34725_EN  ISL29125_EN
//#define TCS34725_INT ISL29125_INT

// Pressure sensor
//#define MS5637_EN   NRF_GPIO_PIN_MAP(1,11)

//Humidity Sensor
//#define SI7021_EN   NRF_GPIO_PIN_MAP(1,10)

// Accelerometer
//#define LI2D_CS     NRF_GPIO_PIN_MAP(0,21)
//#define LI2D_INT1   NRF_GPIO_PIN_MAP(0,20)
//#define LI2D_INT2   NRF_GPIO_PIN_MAP(0,22)

//PIR
#define PIR_EN      NRF_GPIO_PIN_MAP(1,11)
#define PIR_OUT     NRF_GPIO_PIN_MAP(1,10)

//RTC
#define RTC_CS      NRF_GPIO_PIN_MAP(1,12)
#define RTC_IRQ1    NRF_GPIO_PIN_MAP(1,13)
#define RTC_IRQ2    NRF_GPIO_PIN_MAP(1,15)
#define RTC_IRQ3    NRF_GPIO_PIN_MAP(1,14)
#define RTC_WDI     NRF_GPIO_PIN_MAP(0,2)

//ICOUNT
//#define SWITCH      NRF_GPIO_PIN_MAP(0,13)
//#define LBST        NRF_GPIO_PIN_MAP(0,19)

//BATTERIES
#define VSOL        NRF_GPIO_PIN_MAP(0,30)
#define VPRIMARY    NRF_GPIO_PIN_MAP(0,29)
#define VSECONDARY  NRF_GPIO_PIN_MAP(0,31)
#define VBAT_OK     NRF_GPIO_PIN_MAP(0,11)

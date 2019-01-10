// Pin definitions for nRF52 development kit (PCA10040)

#pragma once

#include "nrf_gpio.h"

// LED
#define LEDS_NUMBER 3
#define LEDS_ACTIVE_STATE 0
#define LED_1 NRF_GPIO_PIN_MAP(0,4)
#define LED_2 NRF_GPIO_PIN_MAP(0,5)
#define LED_3 NRF_GPIO_PIN_MAP(0,6)
#define LEDS_INV_MASK LEDS_MASK
#define LEDS_LIST { LED_1, LED_2, LED_3 }
#define BSP_LED_0 LED_1
#define BSP_LED_1 LED_2
#define BSP_LED_2 LED_3

// BUTTONS
#define BUTTONS_NUMBER 0
#define BUTTON_PULL 0

// UART
#define UART_TXD NRF_GPIO_PIN_MAP(0,27)
#define UART_RXD NRF_GPIO_PIN_MAP(0,26)

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
#define RTC_IRQ1    NRF_GPIO_PIN_MAP(1,13)
#define RTC_IRQ2    NRF_GPIO_PIN_MAP(1,14)
#define RTC_IRQ3    NRF_GPIO_PIN_MAP(1,15)
#define RTC_WDI     NRF_GPIO_PIN_MAP(0,2)

//ICOUNT
#define SWITCH      NRF_GPIO_PIN_MAP(0,13)
#define LBST        NRF_GPIO_PIN_MAP(0,19)

//BATTERIES
#define VSOL        NRF_GPIO_PIN_MAP(0,30)
#define VPRIMARY    NRF_GPIO_PIN_MAP(0,29)
#define VSECONDARY  NRF_GPIO_PIN_MAP(0,31)
#define VBAT_OK     NRF_GPIO_PIN_MAP(1,6)

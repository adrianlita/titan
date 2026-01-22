#pragma once

#include <periph/gpio_pins.h>

#define LED                     PB13
#define SW                      PC13

#define DEBUG_UART_TX           PA2
#define DEBUG_UART_RX           PA3
#define DEBUG_UART_BAUDRATE     115200
#define DEBUG_IDLE_PIN          PC8

#define SENSOR_I2C_SCL          PC0
#define SENSOR_I2C_SDA          PC1

#define APDS9301_INT            PA1

#define IR_INPUT                PC9

#define NINA_UART_TX            PC10
#define NINA_UART_RX            PC11
#define NINA_UART_CTS_PIN       PB13    //from the MCU POV
#define NINA_UART_RTS_PIN       PB14
#define NINA_UART_BAUDRATE      115200
#define NINA_RST_PIN            PB8



// #define STP_ENA                 PA5
// #define STP_STEP                PA6
// #define STP_DIR                 PA7

// #define DACOUT_PIN              PA4
// #define AN1_PIN                 PA5     //chann 10
// #define AN2_PIN                 PC3     //chann 4

// #define SPI1_SCK                PA5
// #define SPI1_MISO               PA6
// #define SPI1_MOSI               PA7
// #define SPI1_CS                 PB1

// #define SPI2_SCK                PB13
// #define SPI2_MISO               PB14
// #define SPI2_MOSI               PB15

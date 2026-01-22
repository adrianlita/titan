#pragma once

#include <periph/uart.h>
#include <periph/gpio.h>

#define EXPLORER_COMMAND_PLOT           0x80
#define EXPLORER_COMMAND_ARRPLOT8       0x81
#define EXPLORER_COMMAND_ARRPLOT16      0x82
#define EXPLORER_COMMAND_ARRPLOT32      0x84
#define EXPLORER_COMMAND_LOG            0x83
#define EXPLORER_COMMAND_WAITKEY        0x85
#define EXPLORER_COMMAND_PLOTU          0x86
#define EXPLORER_COMMAND_ARRPLOTU8      0x87
#define EXPLORER_COMMAND_ARRPLOTU16     0x88
#define EXPLORER_COMMAND_ARRPLOTU32     0x89

void explorer_init(uart_t uart, uint32_t uart_baudrate, gpio_pin_t tx_pin, gpio_pin_t rx_pin);
void explorer_deinit(void);

void explorer_write(uint8_t command, uint8_t *data, uint16_t data_length);
uint16_t explorer_read(uint8_t command, uint8_t *data, uint16_t max_data_length);

char explorer_waitkey(void);    //waits for key
char explorer_getkey(void);     //returns key, if key was previously pressed and unprocessed. otherwise returns 0

void explorer_plot8(uint8_t channel, int8_t y);
void explorer_plot16(uint8_t channel, int16_t y);
void explorer_plot32(uint8_t channel, int32_t y);

void explorer_plotu8(uint8_t channel, uint8_t y);
void explorer_plotu16(uint8_t channel, uint16_t y);
void explorer_plotu32(uint8_t channel, uint32_t y);

void explorer_arrplot8(uint8_t channel, int8_t *y, uint16_t length);
void explorer_arrplot16(uint8_t channel, int16_t *y, uint16_t length);
void explorer_arrplot32(uint8_t channel, int32_t *y, uint16_t length);

void explorer_arrplotu8(uint8_t channel, uint8_t *y, uint16_t length);
void explorer_arrplotu16(uint8_t channel, uint16_t *y, uint16_t length);
void explorer_arrplotu32(uint8_t channel, uint32_t *y, uint16_t length);

uint8_t explorer_log(char* format, ...);

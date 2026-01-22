#pragma once

#include <stdint.h>

typedef uint32_t uart_t;

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD,
} uart_parity_t;

typedef enum {
    UART_STOP_BITS_1 = 0,
    UART_STOP_BITS_2
} uart_stop_bits_t;

typedef enum {
    UART_FLOW_CONTROL_NONE = 0,
    UART_FLOW_CONTROL_RTS_CTS,
} uart_flow_control_t;

typedef void(*uart_async_rx_t)(uint8_t data);
typedef void(*uart_async_rx_param_t)(uint32_t param, uint8_t data);

void uart_init(uart_t uart, uint32_t baudrate, uart_parity_t parity, uart_stop_bits_t stop_bits, uart_flow_control_t flow_control, uart_async_rx_t rx_callback);
void uart_init_param(uart_t uart, uint32_t baudrate, uart_parity_t parity, uart_stop_bits_t stop_bits, uart_flow_control_t flow_control, uart_async_rx_param_t rx_callback, uint32_t param);
void uart_deinit(uart_t uart);

void uart_write(uart_t uart, uint8_t *data, uint32_t length);

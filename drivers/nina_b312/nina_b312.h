#pragma once

#include <bluetooth/bluetooth.h>
#include <periph/uart.h>
#include <periph/gpio.h>
#include <task.h>
#include <semaphore.h>
#include <stdbool.h>

#define NINA_B312_STACK_SIZE 512

typedef enum {
    NINA_B312_OK = 0,
    NINA_B312_ERROR = 1,
} nina_b312_rc_t;

typedef enum {
    NINA_B312_STARTUP = 0,
    NINA_B312_READ_REQUEST = 1,
    NINA_B312_WRITE_REQUEST = 2,

    NINA_B312_ACL_CONNECTED,
    NINA_B312_ACL_DISCONNECTED,
    NINA_B312_BLE_PHY_UPDATE,
} nina_b312_urc_t;

typedef struct {
    btif_t *btif;
    gatt_server_t gatt_server;

    uart_t uart;
    gpio_pin_t rst_pin;
    gpio_pin_t tx_pin;
    gpio_pin_t rx_pin;
    gpio_pin_t rts_pin;
    gpio_pin_t cts_pin;

    task_stack_t driver_task_stack[NINA_B312_STACK_SIZE] in_task_stack_memory;
    task_t driver_task;

    uint8_t rx_buffer[256];
    uint8_t rx_buffer_in;
    uint8_t rx_buffer_out;

    semaphore_t rx_unsolicited_received;
    semaphore_t rx_response_received;
    semaphore_t rx_release;

    char rx_command[256];
    uint8_t rx_command_length;
    char rx_content[256];
    nina_b312_rc_t rx_response;
    nina_b312_urc_t rx_uresponse;
} nina_b312_t;

void nina_b312_init(nina_b312_t *dev, btif_t *btif, uart_t uart, gpio_pin_t rst_pin, gpio_pin_t tx_pin, gpio_pin_t rx_pin, gpio_pin_t rts_pin, gpio_pin_t cts_pin);
void nina_b312_deinit(nina_b312_t *dev);

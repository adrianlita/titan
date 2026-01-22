#include "explorer.h"
#include <mutex.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

TITAN_DEBUG_FILE_MARK;

#if (TITAN_EXPLORER_OUTPUT_PACKET_SIZE < 16)
    #error "TITAN_EXPLORER_OUTPUT_PACKET_SIZE should be at least 16"
#endif

static uart_t explorer_uart;
static gpio_pin_t explorer_tx_pin;
static gpio_pin_t explorer_rx_pin;
static mutex_t explorer_write_mutex;
static mutex_t explorer_read_mutex;
static mutex_t explorer_key_mutex;
static semaphore_t explorer_key_semaphore;

static char explorer_output_buffer[TITAN_EXPLORER_OUTPUT_PACKET_SIZE + 8];  //8 is: t2c P LL CC

static enum {
    EXPLORER_RX_STATE_WAIT_FOR_T2C = 0,
    EXPLORER_RX_STATE_WAIT_FOR_METADATA = 1,
    EXPLORER_RX_STATE_WAIT_FOR_DATA = 2,
    EXPLORER_RX_STATE_WAIT_FOR_CHECKSUM = 3,
} explorer_rx_state;
static uint8_t explorer_rx_buffer[16];
static uint16_t explorer_rx_buffer_length;

static uint8_t explorer_rx_command;
static uint16_t explorer_rx_read_length;
static uint16_t explorer_rx_calc_checksum;
static uint16_t explorer_rx_read_checksum;

static char explorer_rx_key;
static uint8_t explorer_rx_key_times;

static struct {
    semaphore_t semaphore;

    uint8_t command;
    uint8_t *data;
    uint16_t max_data_length;
    uint16_t total_data;
} explorer_read_data;

static void explorer_on_rx(uint8_t data) {
    switch (explorer_rx_state) {
        case EXPLORER_RX_STATE_WAIT_FOR_T2C:
            explorer_rx_buffer[explorer_rx_buffer_length++] = data;
            switch (explorer_rx_buffer_length) {
                case 1:
                    if (explorer_rx_buffer[0] != 't') {
                        explorer_rx_buffer_length = 0;
                    }
                    break;

                case 2:
                    if (explorer_rx_buffer[1] != '2') {
                        if (explorer_rx_buffer[1] == 't') {
                            explorer_rx_buffer[0] = 't';
                            explorer_rx_buffer_length = 1;
                        }
                        else {
                            explorer_rx_buffer_length = 0;
                        }
                    }
                    break;

                case 3:
                    if (explorer_rx_buffer[2] != 'c') {
                        if (explorer_rx_buffer[2] == 't') {
                            explorer_rx_buffer[0] = 't';
                            explorer_rx_buffer_length = 1;
                        }
                        else {
                            explorer_rx_buffer_length = 0;
                        }
                    }
                    else {
                        explorer_rx_calc_checksum = 't' + '2' + 'c';
                        explorer_rx_state = EXPLORER_RX_STATE_WAIT_FOR_METADATA;
                    }
                    break;
            }
            break;

        case EXPLORER_RX_STATE_WAIT_FOR_METADATA:
            explorer_rx_calc_checksum += data;
            explorer_rx_buffer[explorer_rx_buffer_length++] = data;
            if (explorer_rx_buffer_length == 6) {
                explorer_rx_read_length = explorer_rx_buffer[4] + 256 * explorer_rx_buffer[5];
                explorer_rx_command = explorer_rx_buffer[3];

                explorer_read_data.total_data = 0;
                if (explorer_rx_read_length == 0) {
                    explorer_rx_read_length = 2;
                    explorer_rx_state = EXPLORER_RX_STATE_WAIT_FOR_CHECKSUM;
                    explorer_rx_read_checksum = 0;
                }
                else {
                    explorer_rx_state = EXPLORER_RX_STATE_WAIT_FOR_DATA;
                }
            }
            break;

        case EXPLORER_RX_STATE_WAIT_FOR_DATA:
            explorer_rx_calc_checksum += data;

            if(explorer_rx_command == EXPLORER_COMMAND_WAITKEY) {
                explorer_rx_buffer[explorer_rx_buffer_length++] = data;
            }
            else if(explorer_rx_command == explorer_read_data.command) {
                if(explorer_read_data.total_data < explorer_read_data.max_data_length) {
                    explorer_read_data.data[explorer_read_data.total_data] = data;
                }
                explorer_read_data.total_data++;
            }

            explorer_rx_read_length--;
            if (explorer_rx_read_length == 0) {
                explorer_rx_read_length = 2;
                explorer_rx_state = EXPLORER_RX_STATE_WAIT_FOR_CHECKSUM;
                explorer_rx_read_checksum = 0;
            }
            break;

        case EXPLORER_RX_STATE_WAIT_FOR_CHECKSUM:
            explorer_rx_read_checksum *= 256;
            explorer_rx_read_checksum += data;
            
            explorer_rx_read_length--;
            if (explorer_rx_read_length == 0) {
                explorer_rx_read_checksum = (explorer_rx_read_checksum >> 8) | (explorer_rx_read_checksum << 8);
                if(explorer_rx_read_checksum == explorer_rx_calc_checksum) {
                    if(explorer_rx_command == EXPLORER_COMMAND_WAITKEY) {
                        explorer_rx_key = explorer_rx_buffer[6];
                        explorer_rx_key_times = 1;
                        if(semaphore_get_waiting_queue(&explorer_key_semaphore)) {
                            semaphore_unlock(&explorer_key_semaphore);
                        }
                    }
                    else if(explorer_rx_command == explorer_read_data.command) {
                        explorer_rx_command = EXPLORER_COMMAND_WAITKEY;
                        semaphore_unlock(&explorer_read_data.semaphore);
                    }
                }

                explorer_rx_state = EXPLORER_RX_STATE_WAIT_FOR_T2C;
                explorer_rx_buffer_length = 0;
            }
            break;
    }
}

static uint16_t _explorer_init_output_header(uint8_t command, uint16_t data_length) {
    explorer_output_buffer[0] = 't';
    explorer_output_buffer[1] = '2';
    explorer_output_buffer[2] = 'c';
    explorer_output_buffer[3] = command;
    explorer_output_buffer[4] = (data_length) % 256;
    explorer_output_buffer[5] = (data_length) / 256;

    uint16_t checksum = 0;
    for(uint8_t i = 0; i < 6; i++) {
        checksum += explorer_output_buffer[i];
    }

    return checksum;
}

void explorer_init(uart_t uart, uint32_t uart_baudrate, gpio_pin_t tx_pin, gpio_pin_t rx_pin) {
    explorer_uart = uart;
    explorer_tx_pin = tx_pin;
    explorer_rx_pin = rx_pin;

    gpio_init_special(explorer_tx_pin, GPIO_SPECIAL_FUNCTION_UART, explorer_uart);
    gpio_init_special(explorer_rx_pin, GPIO_SPECIAL_FUNCTION_UART, explorer_uart);
    uart_init(explorer_uart, uart_baudrate, UART_PARITY_NONE, UART_STOP_BITS_1, UART_FLOW_CONTROL_NONE, explorer_on_rx);

    mutex_create(&explorer_write_mutex);
    mutex_create(&explorer_read_mutex);
    mutex_create(&explorer_key_mutex);
    semaphore_create(&explorer_key_semaphore, 0);

    explorer_rx_command = 0;
    explorer_rx_buffer_length = 0;

    explorer_rx_state = EXPLORER_RX_STATE_WAIT_FOR_T2C;
    explorer_rx_read_length = 0;

    semaphore_create(&explorer_read_data.semaphore, 0);
    explorer_read_data.command = EXPLORER_COMMAND_WAITKEY;
    explorer_read_data.data = 0;
    explorer_read_data.max_data_length = 0;
}

void explorer_deinit(void) {
    gpio_deinit(explorer_tx_pin);
    gpio_deinit(explorer_rx_pin);
    uart_deinit(explorer_uart);
}

void explorer_write(uint8_t command, uint8_t *data, uint16_t data_length) {
    mutex_lock(&explorer_write_mutex);

    uint16_t checksum = _explorer_init_output_header(command, data_length);
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 6);
    uart_write(explorer_uart, data, data_length);
    
    for(uint16_t i = 0; i < data_length; i++) {
        checksum += data[i];
    }

    explorer_output_buffer[0] = (checksum) % 256;
    explorer_output_buffer[1] = (checksum) / 256;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 2);

    mutex_unlock(&explorer_write_mutex);
}

uint16_t explorer_read(uint8_t command, uint8_t *data, uint16_t max_data_length) {
    uint16_t ret_value;

    mutex_lock(&explorer_read_mutex);
    explorer_read_data.command = command;
    explorer_read_data.data = data;
    explorer_read_data.max_data_length = max_data_length;
    semaphore_lock(&explorer_read_data.semaphore);
    ret_value = explorer_read_data.total_data;

    mutex_unlock(&explorer_read_mutex);
    return ret_value;
}

char explorer_waitkey(void) {
    char c;

    mutex_lock(&explorer_key_mutex);
    semaphore_lock(&explorer_key_semaphore);

    c = explorer_rx_key;
    explorer_rx_key_times = 0;

    mutex_unlock(&explorer_key_mutex);
    return c;
}

char explorer_getkey(void) {
    char c = 0;

    mutex_lock(&explorer_key_mutex);

    if(explorer_rx_key_times) {
        c = explorer_rx_key;
        explorer_rx_key_times = 0;
    }

    mutex_unlock(&explorer_key_mutex);
    return c;
}


void explorer_plot8(uint8_t channel, int8_t y) {
    mutex_lock(&explorer_write_mutex);

    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_PLOT, 2);
    explorer_output_buffer[6] = channel;
    explorer_output_buffer[7] = *(uint8_t*)&y;
    
    for(uint16_t i = 6; i < 8; i++) {
        checksum += explorer_output_buffer[i];
    }

    explorer_output_buffer[8] = checksum % 256;
    explorer_output_buffer[9] = checksum / 256;

    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 10);
    mutex_unlock(&explorer_write_mutex);
}

void explorer_plot16(uint8_t channel, int16_t y) {
    mutex_lock(&explorer_write_mutex);

    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_PLOT, 3);
    explorer_output_buffer[6] = channel;
    explorer_output_buffer[7] = *(uint8_t*)&y;
    explorer_output_buffer[8] = *((uint8_t*)&y + 1);
    
    for(uint16_t i = 6; i < 9; i++) {
        checksum += explorer_output_buffer[i];
    }

    explorer_output_buffer[9] = checksum % 256;
    explorer_output_buffer[10] = checksum / 256;

    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 11);
    mutex_unlock(&explorer_write_mutex);
}

void explorer_plot32(uint8_t channel, int32_t y) {
    mutex_lock(&explorer_write_mutex);

    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_PLOT, 5);
    explorer_output_buffer[6] = channel;
    explorer_output_buffer[7] = *(uint8_t*)&y;
    explorer_output_buffer[8] = *((uint8_t*)&y + 1);
    explorer_output_buffer[9] = *((uint8_t*)&y + 2);
    explorer_output_buffer[10] = *((uint8_t*)&y + 3);
    
    for(uint16_t i = 6; i < 11; i++) {
        checksum += explorer_output_buffer[i];
    }

    explorer_output_buffer[11] = checksum % 256;
    explorer_output_buffer[12] = checksum / 256;

    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 13);
    mutex_unlock(&explorer_write_mutex);
}

void explorer_plotu8(uint8_t channel, uint8_t y) {
    mutex_lock(&explorer_write_mutex);

    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_PLOTU, 2);
    explorer_output_buffer[6] = channel;
    explorer_output_buffer[7] = *(uint8_t*)&y;
    
    for(uint16_t i = 6; i < 8; i++) {
        checksum += explorer_output_buffer[i];
    }

    explorer_output_buffer[8] = checksum % 256;
    explorer_output_buffer[9] = checksum / 256;

    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 10);
    mutex_unlock(&explorer_write_mutex);
}

void explorer_plotu16(uint8_t channel, uint16_t y) {
    mutex_lock(&explorer_write_mutex);

    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_PLOTU, 3);
    explorer_output_buffer[6] = channel;
    explorer_output_buffer[7] = *(uint8_t*)&y;
    explorer_output_buffer[8] = *((uint8_t*)&y + 1);
    
    for(uint16_t i = 6; i < 9; i++) {
        checksum += explorer_output_buffer[i];
    }

    explorer_output_buffer[9] = checksum % 256;
    explorer_output_buffer[10] = checksum / 256;

    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 11);
    mutex_unlock(&explorer_write_mutex);
}

void explorer_plotu32(uint8_t channel, uint32_t y) {
    mutex_lock(&explorer_write_mutex);

    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_PLOTU, 5);
    explorer_output_buffer[6] = channel;
    explorer_output_buffer[7] = *(uint8_t*)&y;
    explorer_output_buffer[8] = *((uint8_t*)&y + 1);
    explorer_output_buffer[9] = *((uint8_t*)&y + 2);
    explorer_output_buffer[10] = *((uint8_t*)&y + 3);
    
    for(uint16_t i = 6; i < 11; i++) {
        checksum += explorer_output_buffer[i];
    }

    explorer_output_buffer[11] = checksum % 256;
    explorer_output_buffer[12] = checksum / 256;

    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 13);
    mutex_unlock(&explorer_write_mutex);
}

void explorer_arrplot8(uint8_t channel, int8_t *y, uint16_t length) {
    mutex_lock(&explorer_write_mutex);

    assert(length < 65534);
    
    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_ARRPLOT8, 1 + length);
    explorer_output_buffer[6] = channel;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 7);
    uart_write(explorer_uart, (uint8_t*)y, length);

    checksum += channel;
    for(uint16_t i = 0; i < length; i++) {
        checksum += y[i];
    }

    explorer_output_buffer[0] = (checksum) % 256;
    explorer_output_buffer[1] = (checksum) / 256;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 2);

    mutex_unlock(&explorer_write_mutex);
}

void explorer_arrplot16(uint8_t channel, int16_t *y, uint16_t length) {
    mutex_lock(&explorer_write_mutex);

    uint32_t length2 = length * 2;
    assert(length2 < 65534);
    
    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_ARRPLOT16, 1 + length2);
    explorer_output_buffer[6] = channel;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 7);
    uart_write(explorer_uart, (uint8_t *)y, length2);

    checksum += channel;
    for(uint32_t i = 0; i < length2; i++) {
        checksum += ((uint8_t*)y)[i];
    }

    explorer_output_buffer[0] = (checksum) % 256;
    explorer_output_buffer[1] = (checksum) / 256;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 2);

    mutex_unlock(&explorer_write_mutex);
}

void explorer_arrplot32(uint8_t channel, int32_t *y, uint16_t length) {
    mutex_lock(&explorer_write_mutex);

    uint32_t length4 = length * 4;
    assert(length4 < 65534);
    
    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_ARRPLOT32, 1 + length4);
    explorer_output_buffer[6] = channel;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 7);
    uart_write(explorer_uart, (uint8_t *)y, length4);

    checksum += channel;
    for(uint32_t i = 0; i < length4; i++) {
        checksum += ((uint8_t*)y)[i];
    }

    explorer_output_buffer[0] = (checksum) % 256;
    explorer_output_buffer[1] = (checksum) / 256;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 2);

    mutex_unlock(&explorer_write_mutex);
}

void explorer_arrplotu8(uint8_t channel, uint8_t *y, uint16_t length) {
    mutex_lock(&explorer_write_mutex);

    assert(length < 65534);
    
    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_ARRPLOTU8, 1 + length);
    explorer_output_buffer[6] = channel;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 7);
    uart_write(explorer_uart, y, length);

    checksum += channel;
    for(uint16_t i = 0; i < length; i++) {
        checksum += y[i];
    }

    explorer_output_buffer[0] = (checksum) % 256;
    explorer_output_buffer[1] = (checksum) / 256;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 2);

    mutex_unlock(&explorer_write_mutex);
}

void explorer_arrplotu16(uint8_t channel, uint16_t *y, uint16_t length) {
    mutex_lock(&explorer_write_mutex);

    uint32_t length2 = length * 2;
    assert(length2 < 65534);
    
    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_ARRPLOTU16, 1 + length2);
    explorer_output_buffer[6] = channel;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 7);
    uart_write(explorer_uart, (uint8_t *)y, length2);

    checksum += channel;
    for(uint32_t i = 0; i < length2; i++) {
        checksum += ((uint8_t*)y)[i];
    }

    explorer_output_buffer[0] = (checksum) % 256;
    explorer_output_buffer[1] = (checksum) / 256;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 2);

    mutex_unlock(&explorer_write_mutex);
}

void explorer_arrplotu32(uint8_t channel, uint32_t *y, uint16_t length) {
    mutex_lock(&explorer_write_mutex);

    uint32_t length4 = length * 4;
    assert(length4 < 65534);
    
    uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_ARRPLOTU32, 1 + length4);
    explorer_output_buffer[6] = channel;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 7);
    uart_write(explorer_uart, (uint8_t *)y, length4);

    checksum += channel;
    for(uint32_t i = 0; i < length4; i++) {
        checksum += ((uint8_t*)y)[i];
    }

    explorer_output_buffer[0] = (checksum) % 256;
    explorer_output_buffer[1] = (checksum) / 256;
    uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, 2);

    mutex_unlock(&explorer_write_mutex);
}

uint8_t explorer_log(char* format, ...) {
    mutex_lock(&explorer_write_mutex);

    uint16_t ret_val = 0;    
    va_list arg;
    int r;

    va_start(arg, format);
    r = vsnprintf(explorer_output_buffer + 6, TITAN_EXPLORER_OUTPUT_PACKET_SIZE, format, arg);
    va_end (arg);

    if((r > 0) && (r < TITAN_EXPLORER_OUTPUT_PACKET_SIZE)) {
        #if defined(TITAN_EXPLORER_LOG_PAYLOAD_ONLY)
        uart_write(explorer_uart, (uint8_t *)explorer_output_buffer + 6, r);
        #else
        uint16_t checksum = _explorer_init_output_header(EXPLORER_COMMAND_LOG, r);
        
        r = r + 6;
        for(uint16_t i = 6; i < r; i++) {
            checksum += explorer_output_buffer[i];
        }

        explorer_output_buffer[r] = checksum % 256;
        explorer_output_buffer[r + 1] = checksum / 256;
        uart_write(explorer_uart, (uint8_t *)explorer_output_buffer, r + 2);
        #endif
        ret_val = 1;
    }

    mutex_unlock(&explorer_write_mutex);
    return ret_val;
}

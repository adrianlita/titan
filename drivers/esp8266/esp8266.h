#pragma once

#include <stdint.h>
#include <periph/uart.h>

typedef struct __esp8266 {
    uart_t uart;

} esp8266_t;

void esp8266_init(esp8266_t *dev, uart_t uart);
void esp8266_deinit(void);

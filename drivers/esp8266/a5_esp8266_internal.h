#ifdef USE_ARTIFICIAL5
#ifndef __a5_esp8266_internal_h
#define __a5_esp8266_internal_h

#include <stdint.h>

//a5_esp8266_hardware.c
void a5_esp8266_hw_enable_on();
void a5_esp8266_hw_enable_off();
void a5_esp8266_hw_reset_on();
void a5_esp8266_hw_reset_off();

void a5_esp8266_hw_start_sending();
void a5_esp8266_hw_on_send_complete_isr();
uint8_t a5_esp8266_hw_send_busy();

//a5_esp8266.c
void a5_esp8266_on_time_isr();

uint8_t a5_esp8266_on_tx_isr();
int8_t a5_esp8266_is_tx_empty();

void a5_esp8266_on_rx_isr(const uint8_t c);

#endif
#endif

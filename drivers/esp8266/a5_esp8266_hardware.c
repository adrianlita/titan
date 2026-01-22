#ifdef USE_ARTIFICIAL5
#include "a5_esp8266/a5_esp8266.h"
#include "a5_esp8266/a5_esp8266_internal.h"
#include "local_bsp.h"

static uint8_t a5_esp8266_send_busy = 0;

void a5_esp8266_hw_enable_on()
{
  HAL_GPIO_WritePin(WIFI_Enable_GPIO_Port, WIFI_Enable_Pin, GPIO_PIN_SET);
}

void a5_esp8266_hw_enable_off()
{
  HAL_GPIO_WritePin(WIFI_Enable_GPIO_Port, WIFI_Enable_Pin, GPIO_PIN_RESET);
}

void a5_esp8266_hw_reset_on()
{
  HAL_GPIO_WritePin(WIFI_Reset_GPIO_Port, WIFI_Reset_Pin, GPIO_PIN_RESET);
}

void a5_esp8266_hw_reset_off()
{
  HAL_GPIO_WritePin(WIFI_Reset_GPIO_Port, WIFI_Reset_Pin, GPIO_PIN_SET);
}

void a5_esp8266_hw_start_sending()
{
  __HAL_UART_ENABLE_IT(&a5_esp8266_uart_handle, UART_IT_TXE);
  a5_esp8266_send_busy = 1;
}

void a5_esp8266_hw_on_send_complete_isr()
{
  a5_esp8266_send_busy = 0;
}

uint8_t a5_esp8266_hw_send_busy()
{
  return a5_esp8266_send_busy;
}
#endif

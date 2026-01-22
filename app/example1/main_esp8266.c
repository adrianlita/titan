#include <titan.h>
#include <stdint.h>
#include <task.h>
#include <notification.h>
#include "bsp.h"
#include <periph/gpio.h>
#include <periph/uart.h>
#include <periph/i2c.h>

#include <utils/explorer/explorer.h>

#include <bme680/bme680.h>
#include <apds9301/apds9301.h>
#include <ir_receiver/ir_receiver.h>

#include <esp8266/esp8266.h>

static mutex_t i2c3_mutex;

static bme680_t bme680;
static apds9301_t apds9301;
static esp8266_t esp8266;

static void on_press(void) {
    static int x = 0;
    x ^= 1;
    gpio_digital_write(LED, x);
}

static volatile uint32_t ir_code;
static void ir_on_receive(uint32_t code) {
    ir_code = code;
    notification_send(&main_task, 0x00000001);
}

static volatile uint8_t apds9301_output_enabled;
static void apds9301_on_int(void) {
    notification_send(&main_task, 0x00000002);
}


void app_main(void) {
    //nucleo init
    gpio_init_digital(LED, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);
    gpio_init_interrupt(SW, GPIO_MODE_INTERRUPT_RFEDGE, GPIO_PULL_NOPULL, on_press);
    gpio_init_digital(DEBUG_IDLE_PIN, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);
    
    //explorer init
    gpio_init_special(DEBUG_UART_TX, GPIO_SPECIAL_FUNCTION_UART, (uint32_t)LPUART1);
    gpio_init_special(DEBUG_UART_RX, GPIO_SPECIAL_FUNCTION_UART, (uint32_t)LPUART1);

    explorer_init((uart_t)LPUART1);
    uart_init((uart_t)LPUART1, DEBUG_UART_BAUDRATE, UART_PARITY_NONE, UART_STOP_BITS_1, UART_FLOW_CONTROL_NONE, explorer_on_rx);

    //i2c init
    gpio_init_special(SENSOR_I2C_SCL, GPIO_SPECIAL_FUNCTION_I2C, (uint32_t)I2C3);
    gpio_init_special(SENSOR_I2C_SDA, GPIO_SPECIAL_FUNCTION_I2C, (uint32_t)I2C3);    
    mutex_create(&i2c3_mutex);
    i2c_master_init((i2c_t)I2C3, I2C_BAUDRATE_400KHZ);

    //ESP8266 pins init
    gpio_init_special(ESP_UART_TX, GPIO_SPECIAL_FUNCTION_UART, (uint32_t)USART3);
    gpio_init_special(ESP_UART_RX, GPIO_SPECIAL_FUNCTION_UART, (uint32_t)USART3);
    gpio_init_digital(ESP_EN_PIN, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);
    gpio_init_digital(ESP_RST_PIN, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);

    explorer_log("bme680 returned %d\r\n", bme680_init(&bme680, (i2c_t)I2C3, &i2c3_mutex, 1));
    explorer_log("apds9301 returned %d\r\n", apds9301_init(&apds9301, (i2c_t)I2C3, &i2c3_mutex, APDS9301_ADDR_SEL_FLOAT));
    apds9301_output_enabled = 0;

    uint32_t pending;
    char c = 0;
    bme680_data_t air_data;
    apds9301_data_t apds_data;

    //apds9301 init
    apds9301_turn_on(&apds9301, APDS9301_GAIN_16, APDS9301_INTEGRATION_402MS);
    gpio_init_interrupt(APDS9301_INT, GPIO_MODE_INTERRUPT_FEDGE, GPIO_PULL_NOPULL, apds9301_on_int);
    if(gpio_digital_read(APDS9301_INT) == 0) {
        apds9301_read(&apds9301, &apds_data);
    }

    //ir_receiver
    ir_receiver_t ir_receiver;
    ir_receiver_init(&ir_receiver, IR_INPUT, (timer_t)TIM15, ir_on_receive);

    //esp8266 init
    esp8266_init(&esp8266, (uart_t)USART3);
    uart_init((uart_t)USART3, ESP_UART_BAUDRATE, UART_PARITY_NONE, UART_STOP_BITS_1, UART_FLOW_CONTROL_NONE, xxx);


    uint32_t result = 0;
    notification_set_active(0x0000000F);
    while(1) {
        pending = notification_wait_try(20);

        if(pending & 0x00000001) {
            explorer_log("IR code received: %u\r\n", ir_code);
            switch(ir_code) {
                case 16769055:

                    break;

                case 16736415:

                    break;

                case 16720095:

                    break;

                case 16752735:

                    break;

                case 16748655:

                    break;

                case 16716015:

                    break;

                case 16732335:

                    break;

                case 16764975:

                    break;
            }
        }

        if(pending & 0x00000002) {
            // apds9301_read(&apds9301, &apds_data);
            if(apds9301_output_enabled) {
                explorer_log("APDS9301 read: %d | %d\r\n", apds_data.ch0, apds_data.ch1);
            }
        }


        if((c = explorer_getkey()) != 0) {
            explorer_log("Key was %c\r\n", c);

            switch(c) {
                case 'Q':
                    bme680_read(&bme680, &air_data);
                    explorer_log("BME680 Data: %u kPa | %u deg C | %d RH | %d gas\r\n", air_data.pressure, air_data.temperature, air_data.humidity, air_data.gas_resistance);
                    break;

                case 'W':
                    apds9301_output_enabled ^= 1;
                    explorer_log("APDS9301 Data: %d\r\n", apds9301_output_enabled);
                    break;

                case 'A': {

                } break;

                case 'S': {

                } break;

                case 'D': {

                } break;

                case 'F': {

                } break;

                case 'Z': {

                } break;

                case 'X':

                    break;

                case 'C':

                    break;

                case 'V': {

                } break;

                case 'B': {

                } break;

                case 'N': {

                } break;
            }
        }
    }
}

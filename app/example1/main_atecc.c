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

#include <atecc608/atecc608.h>

static mutex_t i2c3_mutex;

static bme680_t bme680;
static apds9301_t apds9301;

static atecc608_t atecc608;

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

    explorer_log("bme680 returned %d\r\n", bme680_init(&bme680, (i2c_t)I2C3, &i2c3_mutex, 1));
    explorer_log("apds9301 returned %d\r\n", apds9301_init(&apds9301, (i2c_t)I2C3, &i2c3_mutex, APDS9301_ADDR_SEL_FLOAT));
    explorer_log("atecc608 returned %d\r\n", atecc608_init(&atecc608, SENSOR_I2C_SDA, (i2c_t)I2C3, &i2c3_mutex, ATECC608_DEFAULT_I2C_ADDRESS));
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
                        uint8_t pool[ATECC608_RAND_POOL_SIZE];
                        result += atecc608_wakeup(&atecc608);
                        result += atecc608_rand(&atecc608, pool);
                        result += atecc608_idle(&atecc608);
                        explorer_log("result = %d || ", result);

                        for(uint8_t i = 0; i < ATECC608_RAND_POOL_SIZE; i++) {
                            explorer_log("%02x", pool[i]);
                        }

                        explorer_log("\r\n");
                } break;

                case 'S': {
                        uint8_t sha[ATECC608_SHA256_BLOCK_SIZE];
                        result = 0;
                        result += atecc608_wakeup(&atecc608);
                        result += atecc608_sha256_init(&atecc608);
                        result += atecc608_sha256_update(&atecc608, "abcdefghi", 9);
                        result += atecc608_sha256_final(&atecc608, sha);
                        result += atecc608_idle(&atecc608);

                        explorer_log("result = %d || ", result);

                        for(uint8_t i = 0; i < ATECC608_SHA256_BLOCK_SIZE; i++) {
                            explorer_log("%02x", sha[i]);
                        }

                        explorer_log("\r\n");
                } break;

                case 'D': {
                    uint32_t result = 0;
                    uint8_t key_valid[16];

                    result += atecc608_wakeup(&atecc608);

                    for(uint8_t i = 0; i < 16; i++) {
                        key_valid[i] = 0;
                        result += atecc608_get_key_valid(&atecc608, i, key_valid + i);
                        explorer_log("key_valid[%d] = %d\r\n", i, key_valid[i]);
                    }

                    result += atecc608_idle(&atecc608);
                    explorer_log("key_valid total result = %d\r\n", result);
                    
                } break;

                case 'F': {
                    uint32_t result = 0;
                    atecc608_state_t state;

                    result += atecc608_wakeup(&atecc608);
                    result += atecc608_get_state(&atecc608, &state);
                    explorer_log("get_state total result = %d\r\n", result);
                    explorer_log("rng_sram = %d | rng_eeprom = %d | auth_valid = %d | authkey_id = %d\r\n", state.rng_sram, state.rng_eeprom, state.auth_valid, state.authkey_id);
                    explorer_log("tk.valid = %d | .keyid = %d | .sourceflag = %d | gendig_data = %d | genkey_data = %d | nomac_flag = %d\r\n", state.tempkey.valid, state.tempkey.key_id, state.tempkey.source_flag, state.tempkey.gendig_data, state.tempkey.genkey_data, state.tempkey.nomac_flag);
                    result += atecc608_idle(&atecc608);
                    
                    
                } break;

                case 'Z': {
                    uint32_t result = 0;
                    result += atecc608_wakeup(&atecc608);
                    result += atecc608_read_conf(&atecc608);
                    result += atecc608_idle(&atecc608);

                    explorer_log("result = %d\r\n", result);

                    for(uint8_t i = 0; i < ATECC608_CONF_SIZE; i++) {
                        explorer_log("%02X ", atecc608.config.raw[i]);
                        if(i % 32 == 31) {
                            explorer_log("\r\n");
                        }
                    }

                    explorer_log("\r\n");
                } break;

                case 'X':
                    explorer_log("Slot config translated: ");
                    for(uint8_t i = 0; i < 16; i++) {
                        explorer_log("%04X ", atecc608.config.fields.slot_config[i]);
                    }
                    explorer_log("\r\n");

                    explorer_log("Key config translated: ");
                    for(uint8_t i = 0; i < 16; i++) {
                        explorer_log("%04X ", atecc608.config.fields.key_config[i]);
                    }
                    explorer_log("\r\n");
                    break;

                case 'C':
                    atecc608.config.fields.slot_config[9] = 0x0000;

                    explorer_log("slot modif\r\n");
                    break;

                case 'V': {
                    uint32_t result = 0;
                    result += atecc608_wakeup(&atecc608);
                    explorer_log("wakeup result = %d\r\n", result);
                    result += atecc608_write_conf(&atecc608);
                    explorer_log("write conf result = %d\r\n", result);
                    result += atecc608_idle(&atecc608);

                    explorer_log("conf write result = %d || done\r\n", result);
                } break;

                case 'B': {
                    uint32_t result = 0;
                    uint8_t data[72];

                    for(uint8_t i = 0; i < 72; i++) {
                        data[i] = i + 10;
                    }

                    result += atecc608_wakeup(&atecc608);

                    result += atecc608_read_slot(&atecc608, 9, data);
                    explorer_log("read slot result = %d\r\n", result);
                    result += atecc608_idle(&atecc608);

                    explorer_log("w/r idle result = %d || ", result);

                    for(uint8_t i = 0; i < 72; i++) {
                        explorer_log("%02x", data[i]);
                    }
                } break;

                case 'N': {
                    uint32_t result = 0;
                    uint8_t data[72];

                    for(uint8_t i = 0; i < 72; i++) {
                        data[i] = i + 10;
                    }

                    result += atecc608_wakeup(&atecc608);
                    result += atecc608_write_slot(&atecc608, 9, data);
                    explorer_log("write slot result = %d\r\n", result);

                    result += atecc608_read_slot(&atecc608, 9, data);
                    explorer_log("read slot result = %d\r\n", result);
                    result += atecc608_idle(&atecc608);

                    explorer_log("w/r idle result = %d || ", result);

                    for(uint8_t i = 0; i < 72; i++) {
                        explorer_log("%02x", data[i]);
                    }
                } break;
            }
        }
    }
}

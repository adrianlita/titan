#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef enum {
    APDS9301_ADDR_SEL_GND = 0,
    APDS9301_ADDR_SEL_FLOAT = 1,
    APDS9301_ADDR_SEL_VDD = 2,
} apds9301_addr_sel_t;

typedef enum {
    APDS9301_GAIN_1 = 0,
    APDS9301_GAIN_16 = 1,
} apds9301_gain_t;

typedef enum {
    APDS9301_INTEGRATION_13MS7 = 0,
    APDS9301_INTEGRATION_101MS = 1,
    APDS9301_INTEGRATION_402MS = 2,
} apds9301_integration_time_t;

typedef struct __apds9301_data {
    uint16_t ch0;   //visible + ir
    uint16_t ch1;   //ir
} apds9301_data_t;

typedef struct __apds9301 {
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;

    uint8_t revision;

    apds9301_gain_t gain;
    apds9301_integration_time_t integration_time;
} apds9301_t;

uint32_t apds9301_init(apds9301_t *dev, i2c_t i2c, mutex_t *i2c_mutex, apds9301_addr_sel_t addr_sel_value);
uint32_t apds9301_turn_on(apds9301_t *dev, apds9301_gain_t gain, apds9301_integration_time_t integration_time);
uint32_t apds9301_turn_off(apds9301_t *dev);

uint32_t apds9301_read(apds9301_t *dev, apds9301_data_t *data);

/*
    usage:

    #include <apds9301/apds9301.h>

    apds9301_t apds9301;
    apds9301_data_t apds_data;

    ret = apds9301_init(&apds9301, (i2c_t)I2C3, &i2c3_mutex, APDS9301_ADDR_SEL_FLOAT);
    
    apds9301_turn_on(&apds9301, APDS9301_GAIN_16, APDS9301_INTEGRATION_402MS);
    gpio_init_interrupt(APDS9301_INT, GPIO_MODE_INTERRUPT_FEDGE, GPIO_PULL_NOPULL, apds9301_on_int);
    if(gpio_digital_read(APDS9301_INT) == 0) {
        apds9301_read(&apds9301, &apds_data);
    }
*/

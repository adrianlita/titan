#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef enum {
    HMC5983_GAIN_1370 = 0,
    HMC5983_GAIN_1090 = 1,
    HMC5983_GAIN_820 = 2,
    HMC5983_GAIN_660 = 3,
    HMC5983_GAIN_440 = 4,
    HMC5983_GAIN_390 = 5,
    HMC5983_GAIN_330 = 6,
    HMC5983_GAIN_230 = 7,
} hmc5983_gain_t;

typedef enum {
    HMC5983_DATA_RATE_OFF = 8,

    HMC5983_DATA_RATE_0HZ75 = 0,
    HMC5983_DATA_RATE_1HZ5 = 1,
    HMC5983_DATA_RATE_3HZ = 2,
    HMC5983_DATA_RATE_7HZ5 = 3,
    HMC5983_DATA_RATE_15HZ = 4,
    HMC5983_DATA_RATE_30HZ = 5,
    HMC5983_DATA_RATE_75HZ = 6,
    HMC5983_DATA_RATE_220HZ = 7,
} hmc5983_data_rate_t;

typedef struct __hmc5983_data {
    int16_t x;
    int16_t y;
    int16_t z;
} hmc5983_data_t;

typedef struct __hmc5983 {
    i2c_t i2c;
    mutex_t *i2c_mutex;

    hmc5983_gain_t gain;
    hmc5983_data_rate_t data_rate;
} hmc5983_t;

uint32_t hmc5983_init(hmc5983_t *dev, i2c_t i2c, mutex_t *i2c_mutex);
uint32_t hmc5983_turn_on(hmc5983_t *dev, hmc5983_data_rate_t data_rate, hmc5983_gain_t gain);
uint32_t hmc5983_turn_off(hmc5983_t *dev);

uint32_t hmc5983_read(hmc5983_t *dev, hmc5983_data_t *data);

/*
    usage:

    #include <hmc5983/hmc5983.h>

    hmc5983_t hmc5983;
    hmc5983_data_t hm_data;

    ret = hmc5983_init(&hmc5983, (i2c_t)I2C3, &i2c3_mutex);
    
    hmc5983_turn_on(&hmc5983, HMC5983_DATA_RATE_1HZ5, HMC5983_GAIN_1090);
    gpio_init_interrupt(HMC5983_DRDY, GPIO_MODE_INTERRUPT_REDGE, GPIO_PULL_UP, hmc5983_on_int);

    on interrupt: hmc5983_read(&hmc5983, &hm_data);
*/

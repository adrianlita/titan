#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef enum {
  ADXL375_DATA_RATE_0HZ1 = 0,
  ADXL375_DATA_RATE_0HZ2 = 1,
  ADXL375_DATA_RATE_0HZ39 = 2,
  ADXL375_DATA_RATE_0HZ78 = 3,
  ADXL375_DATA_RATE_1HZ56 = 4,
  ADXL375_DATA_RATE_3HZ13 = 5,
  ADXL375_DATA_RATE_6HZ25 = 6,
  ADXL375_DATA_RATE_12HZ5 = 7,
  ADXL375_DATA_RATE_25HZ = 8,
  ADXL375_DATA_RATE_50HZ = 9,
  ADXL375_DATA_RATE_100HZ = 10,
  ADXL375_DATA_RATE_200HZ = 11,
  ADXL375_DATA_RATE_400HZ = 12,
  ADXL375_DATA_RATE_800HZ = 13,
  ADXL375_DATA_RATE_1600HZ = 14,
  ADXL375_DATA_RATE_3200HZ = 15,
} adxl375_data_rate_t;

typedef struct __adxl375_data {
    int16_t x;
    int16_t y;
    int16_t z;
} adxl375_data_t;

typedef struct __adxl375 {
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;

    adxl375_data_rate_t data_rate;
    uint8_t fifo_store;
} adxl375_t;

uint32_t adxl375_init(adxl375_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t sdo_pin_value);
uint32_t adxl375_turn_on(adxl375_t *dev, adxl375_data_rate_t data_rate, uint8_t fifo_store);
uint32_t adxl375_turn_off(adxl375_t *dev);

uint32_t adxl375_read(adxl375_t *dev, adxl375_data_t *data);

/*
    usage:

    #include <adxl375/adxl375.h>

    adxl375_t adxl375;
    adxl375_data_t ad_data[5];

    ret = adxl375_init(&adxl375, (i2c_t)I2C3, &i2c3_mutex, 0);
    gpio_init_interrupt(ADXL375_INT1, GPIO_MODE_INTERRUPT_REDGE, GPIO_PULL_UP, adxl375_on_int);

    adxl375_turn_on(&adxl375, ADXL375_DATA_RATE_1HZ56, 5);
    if(gpio_digital_read(ADXL375_INT1)) {
        adxl375_read(&adxl375, ad_data);
    }
*/

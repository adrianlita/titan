#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef enum {
  LIS3DH_DATARATE_400_HZ = 7,  //  400Hz
  LIS3DH_DATARATE_200_HZ = 6,  //  200Hz
  LIS3DH_DATARATE_100_HZ = 5,  //  100Hz
  LIS3DH_DATARATE_50_HZ = 4,   //   50Hz
  LIS3DH_DATARATE_25_HZ = 3,   //   25Hz
  LIS3DH_DATARATE_10_HZ = 2,   //   10Hz
  LIS3DH_DATARATE_1_HZ = 1,    //    1Hz
  LIS3DH_DATARATE_POWERDOWN = 0,
  LIS3DH_DATARATE_LOWPOWER_1K6HZ = 8,
  LIS3DH_DATARATE_LOWPOWER_5KHZ = 9,
} lis3dh_data_rate_t;

typedef enum {
  LIS3DH_RANGE_16_G = 3,   // +/- 16g
  LIS3DH_RANGE_8_G = 2,    // +/- 8g
  LIS3DH_RANGE_4_G = 1,    // +/- 4g
  LIS3DH_RANGE_2_G = 0     // +/- 2g (default value)
} lis3dh_range_t;

typedef struct __lis3dh_data {
    int16_t x;
    int16_t y;
    int16_t z;
} lis3dh_data_t;

typedef struct __lis3dh {
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;

    lis3dh_data_rate_t data_rate;
    lis3dh_range_t range;
    uint8_t fifo_store;
} lis3dh_t;

uint32_t lis3dh_init(lis3dh_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t sdo_pin_value);
uint32_t lis3dh_turn_on(lis3dh_t *dev, lis3dh_data_rate_t data_rate, lis3dh_range_t range, uint8_t fifo_store);
uint32_t lis3dh_turn_off(lis3dh_t *dev);

uint32_t lis3dh_read(lis3dh_t *dev, lis3dh_data_t *data);

/*
    usage:

    #include <lis3dh/lis3dh.h>

    lis3dh_t lis3dh;
    lis3dh_data_t lis_data[25];

    ret = lis3dh_init(&lis3dh, (i2c_t)I2C3, &i2c3_mutex, 0);
    
    lis3dh_turn_on(&lis3dh, LIS3DH_DATARATE_100_HZ, LIS3DH_RANGE_2_G, 25);
    gpio_init_interrupt(LIS3DH_INT1, GPIO_MODE_INTERRUPT_REDGE, GPIO_PULL_UP, lis3dh_on_int);
    if(gpio_digital_read(LIS3DH_INT1)) {
        lis3dh_read(&lis3dh, lis_data);
    }
*/

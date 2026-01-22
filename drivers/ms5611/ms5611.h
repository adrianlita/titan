#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef enum {
  MS5611_OSR_ULTRA_HIGH_RES   = 0x08,
  MS5611_OSR_HIGH_RES         = 0x06,
  MS5611_OSR_STANDARD         = 0x04,
  MS5611_OSR_LOW_POWER        = 0x02,
  MS5611_OSR_ULTRA_LOW_POWER  = 0x00
} ms5611_osr_t;

typedef struct __ms5611_data {
    uint32_t pressure;
    int32_t temperature;
} ms5611_data_t;

typedef struct __ms5611 {
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;

    ms5611_osr_t osr;
    uint8_t read_time;

    uint16_t c[8];
} ms5611_t;

uint32_t ms5611_init(ms5611_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t csb_pin_value, ms5611_osr_t osr);

uint32_t ms5611_read(ms5611_t *dev, ms5611_data_t *data);

/*
    usage:

    #include <ms5611/ms5611.h>

    ms5611_t ms5611;
    ms5611_data_t ms_data;

    ret = ms5611_init(&ms5611, (i2c_t)I2C3, &i2c3_mutex, 0, MS5611_OSR_ULTRA_HIGH_RES);
    
    ms5611_read(&ms5611, &ms_data);
    explorer_log("MS5611 Data: %d | %d\r\n", ms_data.pressure, ms_data.temperature);
*/
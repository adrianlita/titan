#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef struct __bme680_data {
    int32_t pressure;
    int32_t temperature;
    int32_t humidity;
    int32_t gas_resistance;
} bme680_data_t;

typedef struct __bme680 {
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;
    
    uint8_t ctrl_config;
    uint8_t ctrl_gas0;
    uint8_t ctrl_gas1;
    uint8_t ctrl_hum;
    uint8_t ctrl_meas;

    uint8_t gas_wait;
    uint8_t res_heat;

    //calibration data
    uint16_t par_T1;
    int16_t par_T2;
    int8_t par_T3;

    uint16_t par_P1;
    int16_t par_P2;
    int8_t par_P3;
    int16_t par_P4;
    int16_t par_P5;
    int8_t par_P6;
    int8_t par_P7;
    int16_t par_P8;
    int16_t par_P9;
    uint8_t par_P10;

    uint16_t par_H1;
    uint16_t par_H2;
    int8_t par_H3;
    int8_t par_H4;
    int8_t par_H5;
    uint8_t par_H6;
    int8_t par_H7;

    int8_t par_GH1;
    int16_t par_GH2;
    int8_t par_GH3;

    uint8_t res_heat_range;
    int8_t res_heat_val; 
    int8_t range_switching_error;

    int32_t temp;
} bme680_t;

//checkAL poate fi mult imbunatatit

uint32_t bme680_init(bme680_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t addr_pin_value);
uint32_t bme680_reset(bme680_t *dev);
uint32_t bme680_read(bme680_t *dev, bme680_data_t *data);

/*
    usage:

    #include <bme680/bme680.h>

    bme680_t bme680;
    bme680_data_t air_data;

    ret = bme680_init(&bme680, (i2c_t)I2C3, &i2c3_mutex, 1);
    
    bme680_read(&bme680, &air_data);
    explorer_log("BME680 Data: %u kPa | %u deg C | %d RH | %d gas\r\n", air_data.pressure, air_data.temperature, air_data.humidity, air_data.gas_resistance);
*/

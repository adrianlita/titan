#pragma once

#include <stdint.h>

typedef uint32_t i2c_t;

typedef enum {
    I2C_BAUDRATE_100KHZ = 0,
    I2C_BAUDRATE_400KHZ = 1,
    I2C_BAUDRATE_1000KHZ = 2,
    I2C_BAUDRATE_3400KHZ = 3,
} i2c_baudrate_t;

void i2c_master_init(i2c_t i2c, i2c_baudrate_t baudrate);
void i2c_deinit(i2c_t i2c);

uint32_t i2c_master_read(i2c_t i2c, uint8_t slave_address, void *data, uint32_t length);
uint32_t i2c_master_write(i2c_t i2c, uint8_t slave_address, const void *data, uint32_t length);

uint32_t i2c_master_read_mem(i2c_t i2c, uint8_t slave_address, uint32_t reg, uint8_t reg_size, void *data, uint32_t length);
uint32_t i2c_master_write_mem(i2c_t i2c, uint8_t slave_address, uint32_t reg, uint8_t reg_size, const void *data, uint32_t length);

#pragma once

#include <stdint.h>

typedef uint32_t spi_t;

typedef enum {
    SPI_MODE_0 = 0,         //polarity == 0, phase == 0
    SPI_MODE_1 = 1,         //polarity == 0, phase == 1
    SPI_MODE_2 = 2,         //polarity == 1, phase == 0
    SPI_MODE_3 = 3,         //polarity == 1, phase == 1
} spi_mode_t;

typedef enum {
    SPI_BIT_ORDER_MSB_FIRST = 0,
    SPI_BIT_ORDER_LSB_FIRST = 1,
} spi_bit_order_t;

void spi_master_init(spi_t spi, spi_mode_t mode, uint8_t data_width, spi_bit_order_t bit_order, uint32_t frequency);
void spi_deinit(spi_t spi);

uint32_t spi_master_read(spi_t spi, void *data, uint32_t length);
uint32_t spi_master_write(spi_t spi, const void *data, uint32_t length);
uint32_t spi_master_transfer(spi_t spi, const void *write_data, void *read_data, uint32_t length);

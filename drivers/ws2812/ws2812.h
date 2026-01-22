#pragma once

#include <stdint.h>
#include <periph/spi.h>

//standalone functions
uint32_t ws2812_component_to_rgb(uint8_t red, uint8_t green, uint8_t blue);
void ws2812_rgb_to_msb(uint32_t rgb, uint8_t *msb);    //msb[0] bit 7,6,5 is G7, msb[7] bit 2,1,0 is B0
void ws2812_rgb_to_lsb(uint32_t rgb, uint8_t *lsb);    //lsb[0] bit 7,6,5 is B0, lsb[7] bit 2,1,0 is G7

//spi-based driver
typedef struct __ws2812 {
    spi_t spi;

    uint32_t leds;
    uint8_t *buffer;
    uint32_t buffer_length;
} ws2812_t;

void ws2812_init(ws2812_t *dev, spi_t spi, uint32_t leds, uint8_t *buffer, uint32_t buffer_length);
void ws2812_write(ws2812_t *dev, uint32_t *rgb);

/*
    main idea:
    - 1 effective bit is composed of 3 bits on our side, where each bit is 400ns long:
        - 0 is HLL  -__
        - 1 is HHL  --_
    - 24 bits * 3 = 72 bits --> 9 bytes --> so each LED is 9 bytes long

    - order to be effectively sent to the LED is: G7G6...R7R6..B1B0
    - depending on what it is used, it may be more efficient to use SPI with MSB ordering (when the peripheral does the job, as in this implementation)
    - or some other method, with LSB ordering (for example, bit-banging, where you just get led & 0x01, led >>= 1 and send it for 72 times)


    SPI based driver usage:
    !!! SPI MUST BE CONFIGURED AT 2.5MHz or somewhere near !!!
    !!! LEDS must be connected to the MOSI pin !!!
    !!! Other SPI pins are not used. The SPI can not be used with other devices !!!
    !!! make sure there are 50 us between two writes !!!
    !!! make sure the task doing the write has high priority, so if a lower sized buffer is used, you don't have large yielding !!!

    #include <periph/gpio.h>
    #include <periph/spi.h>
    #include <ws2812/ws2812.h>
    
    gpio_init_special(SPI1_SCK, GPIO_SPECIAL_FUNCTION_SPI, (uint32_t)SPI1);
    gpio_init_special(SPI1_MISO, GPIO_SPECIAL_FUNCTION_SPI, (uint32_t)SPI1);
    gpio_init_special(SPI1_MOSI, GPIO_SPECIAL_FUNCTION_SPI, (uint32_t)SPI1);
    spi_master_init((spi_t)SPI1, SPI_MODE_0, 8, SPI_BIT_ORDER_MSB_FIRST, 2500000);

    static uint8_t ws_buffer[81];   //this number MUST be divisible with 9. the larger the better
    uint32_t leds[144];
    ws2812_t ws2812;
    
    ws2812_init(&ws2812, (spi_t)SPI1, 144, ws_buffer, 81);

    leds[i] = 0xRRGGBB;
            or
    leds[i] = ws2812_component_to_rgb(red, green, blue);
    ws2812_write(&ws2812, leds);
*/

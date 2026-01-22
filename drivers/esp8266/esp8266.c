#include "ws2812.h"
#include <assert.h>

TITAN_DEBUG_FILE_MARK;

static const uint8_t precomputed_MSB[9] = {0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24};
static const uint8_t precomputed_LSB[9] = {0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24};

#define GET_BIT(byte, bit)  ((byte >> bit) & 0x01)
#define PUT_BIT(byte, bit, value) (byte |= (value << bit))

uint32_t ws2812_component_to_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t ret = (red << 16);
    ret |= (green << 8);
    ret |= blue;
    return ret;
}

void ws2812_rgb_to_msb(uint32_t rgb, uint8_t *msb) {
    assert(msb);

    uint8_t red = (rgb & 0xFF0000) >> 16;
    uint8_t green = (rgb & 0xFF00) >> 8;
    uint8_t blue = (rgb & 0xFF);

    for(uint8_t i = 0; i < 9; i++) {
        msb[i] = precomputed_MSB[i];
    }

    PUT_BIT(msb[0], 6, GET_BIT(green, 7));
    PUT_BIT(msb[0], 3, GET_BIT(green, 6));
    PUT_BIT(msb[0], 0, GET_BIT(green, 5));
    PUT_BIT(msb[1], 5, GET_BIT(green, 4));
    PUT_BIT(msb[1], 2, GET_BIT(green, 3));
    PUT_BIT(msb[2], 7, GET_BIT(green, 2));
    PUT_BIT(msb[2], 4, GET_BIT(green, 1));
    PUT_BIT(msb[2], 1, GET_BIT(green, 0));
    PUT_BIT(msb[3], 6, GET_BIT(red, 7));
    PUT_BIT(msb[3], 3, GET_BIT(red, 6));
    PUT_BIT(msb[3], 0, GET_BIT(red, 5));
    PUT_BIT(msb[4], 5, GET_BIT(red, 4));
    PUT_BIT(msb[4], 2, GET_BIT(red, 3));
    PUT_BIT(msb[5], 7, GET_BIT(red, 2));
    PUT_BIT(msb[5], 4, GET_BIT(red, 1));
    PUT_BIT(msb[5], 1, GET_BIT(red, 0));
    PUT_BIT(msb[6], 6, GET_BIT(blue, 7));
    PUT_BIT(msb[6], 3, GET_BIT(blue, 6));
    PUT_BIT(msb[6], 0, GET_BIT(blue, 5));
    PUT_BIT(msb[7], 5, GET_BIT(blue, 4));
    PUT_BIT(msb[7], 2, GET_BIT(blue, 3));
    PUT_BIT(msb[8], 7, GET_BIT(blue, 2));
    PUT_BIT(msb[8], 4, GET_BIT(blue, 1));
    PUT_BIT(msb[8], 1, GET_BIT(blue, 0));
}

void ws2812_rgb_to_lsb(uint32_t rgb, uint8_t *lsb) {
    assert(lsb);

    uint8_t red = (rgb & 0xFF0000) >> 16;
    uint8_t green = (rgb & 0xFF00) >> 8;
    uint8_t blue = (rgb & 0xFF);

    for(uint8_t i = 0; i < 9; i++) {
        lsb[i] = precomputed_LSB[i];
    }

    PUT_BIT(lsb[0], 1, GET_BIT(blue, 0));
    PUT_BIT(lsb[0], 4, GET_BIT(blue, 1));
    PUT_BIT(lsb[0], 7, GET_BIT(blue, 2));
    PUT_BIT(lsb[1], 2, GET_BIT(blue, 3));
    PUT_BIT(lsb[1], 5, GET_BIT(blue, 4));
    PUT_BIT(lsb[2], 0, GET_BIT(blue, 5));
    PUT_BIT(lsb[2], 3, GET_BIT(blue, 6));
    PUT_BIT(lsb[2], 6, GET_BIT(blue, 7));
    PUT_BIT(lsb[3], 1, GET_BIT(red, 0));
    PUT_BIT(lsb[3], 4, GET_BIT(red, 1));
    PUT_BIT(lsb[3], 7, GET_BIT(red, 2));
    PUT_BIT(lsb[4], 2, GET_BIT(red, 3));
    PUT_BIT(lsb[4], 5, GET_BIT(red, 4));
    PUT_BIT(lsb[5], 0, GET_BIT(red, 5));
    PUT_BIT(lsb[5], 3, GET_BIT(red, 6));
    PUT_BIT(lsb[5], 6, GET_BIT(red, 7));
    PUT_BIT(lsb[6], 1, GET_BIT(green, 0));
    PUT_BIT(lsb[6], 4, GET_BIT(green, 1));
    PUT_BIT(lsb[6], 7, GET_BIT(green, 2));
    PUT_BIT(lsb[7], 2, GET_BIT(green, 3));
    PUT_BIT(lsb[7], 5, GET_BIT(green, 4));
    PUT_BIT(lsb[8], 0, GET_BIT(green, 5));
    PUT_BIT(lsb[8], 3, GET_BIT(green, 6));
    PUT_BIT(lsb[8], 6, GET_BIT(green, 7));
}

void ws2812_init(ws2812_t *dev, spi_t spi, uint32_t leds, uint8_t *buffer, uint32_t buffer_length) {
    assert(dev);
    assert(leds);
    assert(buffer);
    assert(buffer_length);
    assert((buffer_length % 9) == 0);

    dev->spi = spi;
    dev->leds = leds;
    dev->buffer = buffer;
    dev->buffer_length = buffer_length;
}

void ws2812_write(ws2812_t *dev, uint32_t *rgb) {
    assert(dev);
    assert(rgb);

    uint32_t led = 0;
    uint32_t buffer_i = 0;

    while(led < dev->leds) {
        ws2812_rgb_to_msb(rgb[led], dev->buffer + buffer_i);
        led++;
        buffer_i += 9;

        if((buffer_i >= dev->buffer_length) || (led >= dev->leds)) {
            spi_master_write(dev->spi, dev->buffer, buffer_i);
            buffer_i = 0;
        }
    }
}

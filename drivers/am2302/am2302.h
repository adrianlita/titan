#pragma once

#include <periph/gpio.h>
#include <periph/timer.h>

typedef struct {
    int16_t temperature;   //divide by 10 to get real temp in C
    int16_t humidity;      //divide by 10 to get real percentage
} am2302_data_t;

typedef struct {
    gpio_pin_t pin;
    timer_t timer;

    uint8_t measuring_edge;
    uint8_t measuring_bit;
    uint16_t old_timer_value;
    uint16_t zero_timing;

    volatile int16_t working_temperature;
    volatile int16_t working_humidity;
    volatile uint8_t working_checksum;
} am2302_t;

void am2302_init(am2302_t *dev, gpio_pin_t pin, timer_t timer);

uint8_t am2302_read(am2302_t *dev, am2302_data_t *data);        //returns 1 if data is valid, 0 otherwise. is a blocking function. should wait >2sec between two calls

/*
    !!! deinit is done automatically after each read(), but there is no need to init() again

    usage:
    #include <am2302/am2302.h>

    am2302_data_t am2302_data;
    am2302_t am2302;
    am2302_init(&am2302, AM2302_PIN, (timer_t)TIM16);

    if(am2302_read(&am2302, &am2302_data)) {
        am2302_data.temperature, am2302_data.humidity are valid
    }


    usage for multiple sensors (timer can be reused):

    am2302_data_t am2302_data;
    
    am2302_t am2302_1;
    am2302_t am2302_2;
    am2302_init(&am2302_1, AM2302_1_PIN, (timer_t)TIM16);
    am2302_init(&am2302_2, AM2302_2_PIN, (timer_t)TIM16);

    if(am2302_read(&am2302_1, &am2302_data)) {
        am2302_data.temperature, am2302_data.humidity are valid
    }

    if(am2302_read(&am2302_2, &am2302_data)) {
        am2302_data.temperature, am2302_data.humidity are valid
    }
*/

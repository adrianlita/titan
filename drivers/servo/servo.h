#pragma once

#include <stdint.h>
#include <periph/gpio.h>

typedef enum {
    SERVO_MODE_50HZ = 0,
    SERVO_MODE_490HZ = 1,
} servo_mode_t;

typedef struct __servo {
    gpio_pin_t pin;
    uint16_t period;
    uint16_t max_duty;
    uint16_t value_us;
} servo_t;

void servo_init(servo_t *dev, gpio_pin_t pin, servo_mode_t mode);
void servo_deinit(servo_t *dev);

void servo_write_us(servo_t *dev, uint16_t us);

#include "servo.h"
#include <periph/cpu.h>
#include <assert.h>

TITAN_DEBUG_FILE_MARK;

void servo_init(servo_t *dev, gpio_pin_t pin, servo_mode_t mode) {
    assert(dev);

    dev->pin = pin;
    dev->value_us = 0;

    gpio_init_pwm(pin, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL, 0);
    switch(mode) {
        case SERVO_MODE_50HZ:
            dev->period = 20000;
            gpio_pwm_write_duty(pin, 0);
            gpio_pwm_write_frequency(pin, 50);
            break;

        case SERVO_MODE_490HZ:
            dev->period = 2041;
            gpio_pwm_write_duty(pin, 0);
            gpio_pwm_write_frequency(pin, 490);
            break;

        default:
            assert(0);
            break;
    }

    dev->max_duty = gpio_pwm_get_max_duty(pin);
}

void servo_deinit(servo_t *dev) {
    assert(dev);

    gpio_deinit(dev->pin);
}


void servo_write_us(servo_t *dev, uint16_t us) {
    assert(dev);

    dev->value_us = us;
    us = (uint32_t)us*dev->max_duty / dev->period;
    gpio_pwm_write_duty(dev->pin, us);
}


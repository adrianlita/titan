#pragma once

#include <stdint.h>
#include <periph/gpio_pins.h>

typedef uint32_t gpio_pin_t;

typedef enum {
    GPIO_PULL_NOPULL = 0,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN
} gpio_pull_t;

typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT_PP = 1,
    GPIO_MODE_OUTPUT_OD = 2,
} gpio_digital_mode_t;

typedef enum {
    GPIO_MODE_INTERRUPT_REDGE = 3,
    GPIO_MODE_INTERRUPT_FEDGE = 4,
    GPIO_MODE_INTERRUPT_RFEDGE = 5,
} gpio_interrupt_mode_t;

typedef enum {
    GPIO_MODE_ANALOG_IN = 6,
    GPIO_MODE_ANALOG_OUT = 7,
} gpio_analog_mode_t;

typedef enum {
    GPIO_SPECIAL_FUNCTION_I2C = 8,
    GPIO_SPECIAL_FUNCTION_SDMMC = 9,
    GPIO_SPECIAL_FUNCTION_SPI = 10,
    GPIO_SPECIAL_FUNCTION_TIMER = 11,
    GPIO_SPECIAL_FUNCTION_UART = 12,
    GPIO_SPECIAL_FUNCTION_USB = 13,
} gpio_special_function_t;

typedef void(*gpio_isr_t)(void);
typedef void(*gpio_isr_param_t)(uint32_t param);

void gpio_init_digital(gpio_pin_t pin, gpio_digital_mode_t mode, gpio_pull_t pull);
void gpio_init_interrupt(gpio_pin_t pin, gpio_interrupt_mode_t mode, gpio_pull_t pull, gpio_isr_t callback);
void gpio_init_interrupt_param(gpio_pin_t pin, gpio_interrupt_mode_t mode, gpio_pull_t pull, gpio_isr_param_t callback, uint32_t param);
void gpio_init_analog(gpio_pin_t pin, gpio_analog_mode_t mode, uint32_t bits);
void gpio_init_special(gpio_pin_t pin, gpio_special_function_t function, uint32_t param);
void gpio_init_pwm(gpio_pin_t pin, gpio_digital_mode_t mode, gpio_pull_t pull, uint32_t initial_value);
void gpio_deinit(gpio_pin_t pin);

uint32_t gpio_digital_read(gpio_pin_t pin);
void gpio_digital_write(gpio_pin_t pin, uint32_t value);
void gpio_digital_toggle(gpio_pin_t pin);

uint32_t gpio_analog_read(gpio_pin_t pin);
void gpio_analog_write(gpio_pin_t pin, uint32_t value);

void gpio_pwm_write_frequency(gpio_pin_t pin, uint32_t frequency);
void gpio_pwm_write_duty(gpio_pin_t pin, uint32_t duty);
uint32_t gpio_pwm_get_max_duty(gpio_pin_t pin);

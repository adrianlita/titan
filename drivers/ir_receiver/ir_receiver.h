#pragma once

#include <periph/gpio.h>
#include <periph/timer.h>

typedef void(*ir_remote_on_receive_t)(uint32_t code);

typedef struct {
    ir_remote_on_receive_t on_receive;
    gpio_pin_t pin; 
    timer_t timer;

    volatile uint8_t learning_state;
    volatile uint8_t learning_bit;
    volatile uint32_t code_learned;
    volatile uint32_t old_timer_value;
} ir_receiver_t;

void ir_receiver_init(ir_receiver_t *dev, gpio_pin_t pin, timer_t timer, ir_remote_on_receive_t on_receive);
void ir_receiver_deinit(ir_receiver_t *dev);

/*
    usage:

    #include <ir_receiver/ir_receiver.h>

    static volatile uint32_t ir_code;
    static void ir_on_receive(uint32_t code) {
        ir_code = code;
        //notifty the task about the async event
    }

    ir_receiver_t ir_receiver;
    ir_receiver_init(&ir_receiver, IR_INPUT, (timer_t)TIM15, ir_on_receive);
*/

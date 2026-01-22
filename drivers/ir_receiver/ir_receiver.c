#include "ir_receiver.h"
#include <periph/cpu.h>
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

static void ir_receiver_isr(ir_receiver_t *dev) {
    assert(dev);

    uint32_t current_timer_value = timer_read(dev->timer);
    uint8_t bit_value = (gpio_digital_read(dev->pin) == 0);
    uint32_t current_value = current_timer_value - dev->old_timer_value;
    dev->old_timer_value = current_timer_value;

    switch(dev->learning_state) {
        case 0: //asteapta un bit de 0, valoare 90
            if((bit_value == 0) && (current_value > 40) && (current_value < 95)) {
                dev->code_learned = 0;
                dev->learning_bit = 0;
                dev->learning_state++;
            }
            break;

        case 1: //asteapta un bit de 1, valoare 45
            if((bit_value == 1) && (current_value > 40) && (current_value < 50)) {
                dev->learning_state++;
            }
            else {
                dev->learning_state = 0;
            }
            break;

        case 2:     //asteapta un bit de 0, valoare 5
            if((bit_value == 0) && (current_value > 3) && (current_value < 10)) {
                dev->learning_state++;
            }
            else {
                dev->learning_state = 0;
            }
            break;

        case 3:     //asteapta un bit de 1, valoare 5-15
            if((bit_value == 1) && (current_value > 3) && (current_value < 20)) {
                dev->learning_state = 2;
                dev->code_learned <<= 1;
                dev->code_learned += (current_value > 10);
                dev->learning_bit++;
                if(dev->learning_bit == 32) {
                    dev->on_receive(dev->code_learned);
                }
            }
            else {
                dev->learning_state = 0;
            }
            break;
    }
}

void ir_receiver_init(ir_receiver_t *dev, gpio_pin_t pin, timer_t timer, ir_remote_on_receive_t on_receive) {
    assert(dev);
    assert(on_receive);

    dev->on_receive = on_receive;
    dev->pin = pin;
    dev->timer = timer;

    dev->learning_state = 0;
    dev->learning_bit = 0;
    dev->code_learned = 0;
    dev->old_timer_value = 0;

    gpio_init_interrupt_param(dev->pin, GPIO_MODE_INTERRUPT_RFEDGE, GPIO_PULL_UP, (gpio_isr_param_t)ir_receiver_isr, (uint32_t)dev);
    timer_init(dev->timer, cpu_clock_speed() / 10000, 0);
    timer_start(dev->timer, 0xFFFF);
}

void ir_receiver_deinit(ir_receiver_t *dev) {
    assert(dev);

    timer_stop(dev->timer);
    timer_deinit(dev->timer);
    gpio_deinit(dev->pin);
}

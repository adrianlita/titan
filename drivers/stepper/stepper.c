#include "stepper.h"
#include <periph/cpu.h>
#include <kernel.h>
#include <assert.h>

TITAN_DEBUG_FILE_MARK;

#define STEPPER_TIMER_FREQ  1000000     //do not modify

#define STEPPER_STATE_STOP      0
#define STEPPER_STATE_ACCEL     1
#define STEPPER_STATE_DECEL     2
#define STEPPER_STATE_RUN       3

static uint32_t fast_sqrt(uint32_t x);
static void stepper_do_step(stepper_t *dev);
static void stepper_enable(stepper_t *dev);
static void stepper_disable(stepper_t *dev);
static void stepper_set_direction(stepper_t *dev);

static void stepper_isr(stepper_t *dev) {
    uint32_t new_step_delay;

    switch(dev->state) {
        case STEPPER_STATE_STOP :
            timer_stop(dev->timer);
            if(dev->zero_speed_hold == 0) {
                stepper_disable(dev);
            }

            if(dev->task) {
                kernel_scheduler_io_wait_finished(dev->task);
                kernel_scheduler_trigger();
            }
            break;

        case STEPPER_STATE_ACCEL:
            timer_change_period(dev->timer, dev->step_delay);
            stepper_do_step(dev);
            dev->steps_taken++;
            if(dev->steps_positive) {
                dev->position++;
            }
            else {
                dev->position--;
            }
            dev->accel_decel_count++;
            new_step_delay = dev->step_delay - (((2 * (int64_t)dev->step_delay) + dev->rest) / (4 * dev->accel_decel_count + 1));
            dev->rest = ((2 * (int64_t)dev->step_delay) + dev->rest) % (4 * dev->accel_decel_count + 1);
            if(dev->steps_taken >= dev->decel_start) {
                dev->accel_decel_count = dev->decel_val;
                dev->state = STEPPER_STATE_DECEL;
            }
            else if(new_step_delay <= dev->min_delay) {
                dev->last_accel_delay = new_step_delay;
                new_step_delay = dev->min_delay;
                dev->rest = 0;
                dev->state = STEPPER_STATE_RUN;
            }
            else if(new_step_delay > 65535) {
                dev->last_accel_delay = new_step_delay;
                new_step_delay = 65535;
            }
            break;

        case STEPPER_STATE_RUN  :
            timer_change_period(dev->timer, dev->step_delay);
            stepper_do_step(dev);
            dev->steps_taken++;
            if(dev->steps_positive) {
                dev->position++;
            }
            else {
                dev->position--;
            }
            new_step_delay = dev->min_delay;
            if(dev->steps_taken >= dev->decel_start) {
                dev->accel_decel_count = dev->decel_val;
                new_step_delay = dev->last_accel_delay;
                dev->state = STEPPER_STATE_DECEL;
            }

            break;

        case STEPPER_STATE_DECEL:
            timer_change_period(dev->timer, dev->step_delay);
            stepper_do_step(dev);
            dev->steps_taken++;
            if(dev->steps_positive) {
                dev->position++;
            }
            else {
                dev->position--;
            }
            dev->accel_decel_count++;
            new_step_delay = dev->step_delay - (((2 * (int64_t)dev->step_delay) + dev->rest) / (4 * dev->accel_decel_count + 1));
            dev->rest = ((2 * (int64_t)dev->step_delay) + dev->rest) % (4 * dev->accel_decel_count + 1);
            if(dev->accel_decel_count >= 0) {
                dev->state = STEPPER_STATE_STOP;
            }
            else if(new_step_delay > 65535) {
                dev->last_accel_delay = new_step_delay;
                new_step_delay = 65535;
            }
            break;
    }

    dev->step_delay = new_step_delay;
}

void stepper_init(stepper_t *dev, gpio_pin_t step, gpio_pin_t dir, gpio_pin_t ena, uint8_t polarity, timer_t timer) {
    assert(dev);

    dev->step_pin = step;
    dev->dir_pin = dir;
    dev->ena_pin = ena;
    dev->polarity = polarity;
    dev->timer = timer;

    dev->state = STEPPER_STATE_STOP;
    dev->position = 0;
    dev->zero_speed_hold = 0;
    dev->accel_spsps = 1;
    dev->decel_spsps = 1;
    dev->min_delay = 65535;
    dev->first_step_delay = 65535;
    dev->steps_to_max = 1;
    dev->decel_val_std = -1;

    gpio_init_digital(step, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);
    gpio_init_digital(dir, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);
    gpio_init_digital(ena, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);

    stepper_disable(dev);

    timer_init_param(timer, cpu_clock_speed() / STEPPER_TIMER_FREQ, (timer_isr_param_t)stepper_isr, (uint32_t)dev);
}

void stepper_deinit(stepper_t *dev) {
    assert(dev);

    timer_deinit(dev->timer);
    gpio_deinit(dev->step_pin);
    gpio_deinit(dev->dir_pin);
    gpio_deinit(dev->ena_pin);
}

void stepper_set_param(stepper_t *dev, uint8_t zero_speed_hold, uint32_t max_speed_sps, uint32_t accel_spsps, uint32_t decel_spsps) {
    assert(dev);

    int32_t min_period = STEPPER_TIMER_FREQ / max_speed_sps;
    assert((min_period != 0) && (min_period <= 65535));

    dev->zero_speed_hold = zero_speed_hold;
    dev->accel_spsps = accel_spsps;
    dev->decel_spsps = decel_spsps;

    dev->min_delay = min_period;
    dev->first_step_delay = fast_sqrt(2 * STEPPER_TIMER_FREQ / accel_spsps) * 676;

    assert(dev->first_step_delay <= 65535);

    dev->steps_to_max = (uint64_t)max_speed_sps*max_speed_sps/(uint64_t)(2 * accel_spsps);
    if(dev->steps_to_max == 0) {
        dev->steps_to_max = 1;
    }
    dev->decel_val_std = -((int64_t)dev->steps_to_max * accel_spsps) / decel_spsps;

    if(zero_speed_hold) {
        stepper_enable(dev);
    }
}

void stepper_step(stepper_t *dev, int32_t steps) {
    assert(dev);
    assert(steps != 0);

    int32_t steps_copy = steps;

    if(dev->zero_speed_hold == 0) {
        stepper_enable(dev);
    }

    dev->steps_positive = (steps > 0);
    stepper_set_direction(dev);
    if(steps < 0) {
        steps = -steps;
    }

    if(steps == 1) {
        stepper_do_step(dev);
        dev->position += steps_copy;
    }
    else {
        volatile kernel_atomic_t __atomic;
        kernel_begin_critical(&__atomic);
        dev->task = (task_t*)kernel_current_task;

        dev->steps_taken = 0;
        dev->accel_decel_count = 0;

        dev->step_delay = dev->first_step_delay;

        uint32_t accel_lim = ((uint64_t)steps*dev->decel_spsps) / (dev->accel_spsps + dev->decel_spsps);
        if(accel_lim == 0) {
            accel_lim = 1;
        }

        if(accel_lim <= dev->steps_to_max) {
            dev->decel_val = accel_lim - steps;
        }
        else {
            dev->decel_val = dev->decel_val_std;
        }

        if(dev->decel_val == 0) {
            dev->decel_val = -1;
        }

        dev->decel_start = dev->decel_val + steps;

        if(dev->step_delay <= dev->min_delay) {
            dev->step_delay = dev->min_delay;
            dev->state = STEPPER_STATE_RUN;
        }
        else {
            dev->state = STEPPER_STATE_ACCEL;
        }

        timer_clear(dev->timer);
        timer_start(dev->timer, dev->step_delay);
        kernel_scheduler_io_wait_start();
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
    }
}

void stepper_step_start(stepper_t *dev, int32_t steps) {
    assert(dev);
    assert(steps != 0);

    int32_t steps_copy = steps;

    if(dev->zero_speed_hold == 0) {
        stepper_enable(dev);
    }

    dev->steps_positive = (steps > 0);
    stepper_set_direction(dev);
    if(steps < 0) {
        steps = -steps;
    }

    if(steps == 1) {
        stepper_do_step(dev);
        dev->position += steps_copy;
    }
    else {
        dev->task = 0;

        dev->steps_taken = 0;
        dev->accel_decel_count = 0;

        dev->step_delay = dev->first_step_delay;

        uint32_t accel_lim = ((uint64_t)steps*dev->decel_spsps) / (dev->accel_spsps + dev->decel_spsps);
        if(accel_lim == 0) {
            accel_lim = 1;
        }

        if(accel_lim <= dev->steps_to_max) {
            dev->decel_val = accel_lim - steps;
        }
        else {
            dev->decel_val = dev->decel_val_std;
        }

        if(dev->decel_val == 0) {
            dev->decel_val = -1;
        }

        dev->decel_start = dev->decel_val + steps;

        if(dev->step_delay <= dev->min_delay) {
            dev->step_delay = dev->min_delay;
            dev->state = STEPPER_STATE_RUN;
        }
        else {
            dev->state = STEPPER_STATE_ACCEL;
        }

        timer_clear(dev->timer);
        timer_start(dev->timer, dev->step_delay);
    }
}

uint8_t stepper_get_running(stepper_t *dev) {
    assert(dev);

    return (dev->state != STEPPER_STATE_STOP);
}

void stepper_clear_position(stepper_t *dev) {
    assert(dev);
    dev->position = 0;
}

int32_t stepper_get_position(stepper_t *dev) {
    assert(dev);
    return dev->position;
}

// sqrt routine 'grupe', from comp.sys.ibm.pc.programmer
// Subject: Summary: SQRT(int) algorithm (with profiling)
//    From: warwick@cs.uq.oz.au (Warwick Allison)
//    Date: Tue Oct 8 09:16:35 1991
static uint32_t fast_sqrt(uint32_t x) {
    uint32_t xr;    // result register
    uint32_t q2;    // scan-bit register
    uint8_t f;      // flag (one bit)

    xr = 0;                     // clear result
    q2 = 0x40000000L;           // higest possible result bit
    do {
        if((xr + q2) <= x) {
            x -= xr + q2;
            f = 1;                  // set flag
        }
        else {
            f = 0;                  // clear flag
        }

        xr >>= 1;
        if(f) {
            xr += q2;               // test flag
        }
    } while(q2 >>= 2);              // shift twice
    
    if(xr < x) {
        return xr + 1;              // add for rounding
    }
    else {
        return xr;
    }
}

static void stepper_do_step(stepper_t *dev) {
    gpio_digital_toggle(dev->step_pin);
    gpio_digital_toggle(dev->step_pin);
}

static void stepper_enable(stepper_t *dev) {
    if(dev->polarity & STEPPER_ENA_POLARITY_INVERTED) {
        gpio_digital_write(dev->ena_pin, 0);
    }
    else {
        gpio_digital_write(dev->ena_pin, 1);
    }    
}

static void stepper_disable(stepper_t *dev) {
    if(dev->polarity & STEPPER_ENA_POLARITY_INVERTED) {
        gpio_digital_write(dev->ena_pin, 1);
    }
    else {
        gpio_digital_write(dev->ena_pin, 0);
    }
}

static void stepper_set_direction(stepper_t *dev) {
    uint8_t direction = dev->steps_positive;

    if(dev->polarity & STEPPER_DIR_POLARITY_INVERTED) {
        direction = !direction;
    }

    gpio_digital_write(dev->dir_pin, direction);
}

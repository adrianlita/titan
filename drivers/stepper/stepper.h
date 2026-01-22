#pragma once

#include <stdint.h>
#include <task.h>
#include <periph/gpio.h>
#include <periph/timer.h>

#define STEPPER_ENA_POLARITY_NORMAL         0x00
#define STEPPER_ENA_POLARITY_INVERTED       0x01

#define STEPPER_DIR_POLARITY_NORMAL         0x00
#define STEPPER_DIR_POLARITY_INVERTED       0x02

typedef struct __stepper {
    gpio_pin_t step_pin;
    gpio_pin_t dir_pin;
    gpio_pin_t ena_pin;
    uint8_t polarity;
    timer_t timer;
    task_t *task;

    //motor parameters
    uint8_t zero_speed_hold;
    uint32_t accel_spsps;       //in steps/s^2
    uint32_t decel_spsps;
    int32_t min_delay;          //min delay between two steps, in timer counts

    //calculated parameteres
    uint32_t first_step_delay;  //delay between the two first steps, in timer counts
    uint32_t steps_to_max;      //number of steps for acceleration (0 to max_speed)
    int32_t decel_val_std;      //number of steps from full speed to 0, negated

    int32_t decel_val;          //number of total deceleration steps
    uint32_t decel_start;       //moment when deceleration starts, in number of steps
    int32_t accel_decel_count;  //number of acceleration steps taken (when positive), or deceleration steps to be taken (when negative)
    int32_t last_accel_delay;   //used to store step delay if step delay is smaller then min_delay
    uint32_t rest;              //running counter used to store division errors

    uint8_t steps_positive;     //1 if steps are positive, 0 if negative
    uint32_t step_delay;        //current delay between steps
    uint32_t steps_taken;       //number of steps already taken

    uint8_t state;              //stepper state, internal variable
    int32_t position;           //stepper position
} stepper_t;

void stepper_init(stepper_t *dev, gpio_pin_t step, gpio_pin_t dir, gpio_pin_t ena, uint8_t polarity, timer_t timer);
void stepper_deinit(stepper_t *dev);

void stepper_set_param(stepper_t *dev, uint8_t zero_speed_hold, uint32_t max_speed_sps, uint32_t accel_spsps, uint32_t decel_spsps);
void stepper_step(stepper_t *dev, int32_t steps);               //take number of steps - blocking

void stepper_step_start(stepper_t *dev, int32_t steps);         //take number of steps - non-blocking. check finished with _get_running, and instantaneous position with _get_position
uint8_t stepper_get_running(stepper_t *dev);                    //returns 1 if stepper is moving, 0 if still

void stepper_clear_position(stepper_t *dev);                    //clear stepper absolute position
int32_t stepper_get_position(stepper_t *dev);                   //stepper absolute position
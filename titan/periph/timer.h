#pragma once

#include <stdint.h>

typedef uint32_t timer_t;

typedef void(*timer_isr_t)(void);
typedef void(*timer_isr_param_t)(uint32_t param);

void timer_init(timer_t timer, uint32_t prescaler, timer_isr_t callback);
void timer_init_param(timer_t timer, uint32_t prescaler, timer_isr_param_t callback, uint32_t param);
void timer_deinit(timer_t timer);

void timer_start(timer_t timer, uint32_t period);
void timer_stop(timer_t timer);
void timer_change_period(timer_t timer, uint32_t new_period);

void timer_clear(timer_t timer);
uint32_t timer_read(timer_t timer);

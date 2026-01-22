#pragma once

#include <kernel_defines.h>
#include <kernel_atomic.h>

/* task attributes */
void task_init(task_t *task, task_stack_t *stack, uint32_t stack_size, uint8_t priority) K_ISR_SAFE K_CSECT_NOMODIF;      //initialize task. must be called before create
void task_attr_set_start_type(task_t *task, task_start_type_t start_type, uint32_t timeout) K_ISR_SAFE K_CSECT_NOMODIF;   //set start type (optional)

/* task creation variants */
void task_create(task_t *task, void(*start_routine)(void)) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                    //creates new task with no arguments and no value returned
void task_create_a(task_t *task, void(*start_routine)(void*), void *arg) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;      //creates new task that has arguments
void task_create_ra(task_t *task, void*(*start_routine)(void*), void *arg) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;    //creates new task that returns values and has arguments

/* task start variants */
void task_start(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                  //start task from the begining (by another task) with no arguments. task must not be running
void task_start_a(task_t *task, void *arg) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;     //start task from the begining with (by another task) arguments. task must not be running

/* task exit variants (also "return" is permitted and will automatically call one of these) */
void task_exit(void) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                    //task decides to exit
void task_exit_r(void *retval) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;          //task decides to exit, with returning arguments

/* task kill variants */
void task_kill(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;            //kill task (by another task), with no return value
void task_kill_r(task_t *task, void *ret) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;            //kill task (by another task), with return value

/* task state information */
uint8_t task_ready(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;               //return 1 if task is running (ready or running), 0 else
uint8_t task_started(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;             //return 1 if task is started (not TS_FINISHED or TS_STOPPED), 0 else

uint8_t task_waiting(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;             //return 1 if task is block-waiting on a primitive/sleep, 0 else
uint8_t task_sleeping(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;            //returns 1 if task is sleeping, 0 otherwise. returns 1 also when another block-wait is ongoing (ex primitive_lock_try)

uint8_t task_stopped(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;             //return 1 if task stopped (not started), 0 else
uint8_t task_exited(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;              //return 1 if task exited (via task_exit or return), 0 else
uint8_t task_killed(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;              //return 1 if task was killed, 0 else
uint8_t task_finished(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;            //return 1 if task was finished (TS_FINISHED == Killed | Exited), 0 else

/* returns currently running task */
task_t *task_self(void) K_ISR_SAFE K_CSECT_NOMODIF;


/* task priority */
uint8_t task_priority_get(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                            //return task priority. if task is null, returns own priority
void task_priority_set(task_t *task, uint8_t priority) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;             //sets task priority, taking effect after task is rescheduled
void task_priority_set_immediate(task_t *task, uint8_t priority) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;   //sets task priority, taking effect immediately. does more kernel work

/* task yielding */
void task_yield(void) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                            //task decides it does not need to run this pass and voluntarily yields the processor to another task

/* task join */
void task_join(task_t *task, void **retval) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                          //join task (wait until task finishes). if task is created but not started, will wait until task starts and finishes
uint8_t task_join_try(task_t *task, void **retval, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT; //join task (wait until task finishes --returns 1, or timeout expires--returns 0). timeout 0 means no waiting at all if the task is not yet terminated. if task is created but not started, will wait until task starts and finishes

/* task sleep */
void task_sleep(uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                                    //sleep for "timeout" ticks
void task_sleep_periodic(uint32_t *timer_buffer, uint32_t period) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;    //variable sleep ammount so that task execution is periodic to "period"
void task_wake(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                                       //abort sleeping for task (if sleeping)

/* task diagnostic functions */
uint32_t task_info_stack_get_total(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;    //return the total number of stack elements (uint32_t). if task == 0, task is current_task
uint32_t task_info_stack_get_used(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;     //return the number of stack elements (uint32_t) currently used on the stack. if task == 0, task is current_task
uint32_t task_info_stack_get_maxused(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;  //return the number of stack elements (uint32_t) that were need to be placed on the stack. if task == 0, task is current_task

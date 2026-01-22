#pragma once

#include <kernel_defines.h>
#include <kernel_atomic.h>
#include <stdint.h>

typedef struct {
    primitive_t parent;
    task_t *owner;
    uint32_t locks;
} mutex_t;

void mutex_create(mutex_t *mtx) K_ISR_FORBID K_CSECT_NOMODIF;                                       //initialize mutex
uint32_t mutex_get_locked(mutex_t *mtx) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                      //get mutex lock count (0 == free, otherwise == locked)
task_t* mutex_get_waiting_queue(mutex_t *mtx) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;              //get mutex waiting queue (0 == nobody)

void mutex_lock(mutex_t *mtx) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                              //get mutex lock, waiting forever
uint8_t mutex_lock_try(mutex_t *mtx, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;     //try to get mutex lock, waiting "timeout" ticks. returns 1 when aquired, 0 when timed out. if timeout is 0, will return immediately. if mutex is flushed while waiting, will return 0
void mutex_unlock(mutex_t *mtx) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                            //unlock the mutex by adding 1 to its value

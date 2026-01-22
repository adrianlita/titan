#pragma once

#include <kernel_defines.h>
#include <kernel_atomic.h>
#include <stdint.h>

typedef struct {
    primitive_t parent;
    uint32_t tokens;
} semaphore_t;

void semaphore_create(semaphore_t *sem, uint32_t initial_tokens) K_ISR_SAFE K_CSECT_NOMODIF;                //initialize semaphore
uint32_t semaphore_get_token_count(semaphore_t *sem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                 //get semaphore token count (0 == locked, otherwise == unlocked)
task_t* semaphore_get_waiting_queue(semaphore_t *sem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;              //get semaphore waiting queue (0 == nobody is waiting)

void semaphore_lock(semaphore_t *sem) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                              //get semaphore lock, waiting forever
uint8_t semaphore_lock_try(semaphore_t *sem, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;     //try to get semaphore lock, waiting "timeout" ticks. returns 1 when aquired, 0 when timed out. if timeout is 0, will return immediately. if semaphore is flushed while waiting, will return 0
void semaphore_unlock(semaphore_t *sem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                              //unlock the semaphore by adding 1 to its value

void semaphore_multiunlock(semaphore_t *sem, uint32_t tokens) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;     //unlock the semaphore by adding "tokens" to its value

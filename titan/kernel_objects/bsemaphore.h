#pragma once

#include <kernel_defines.h>
#include <kernel_atomic.h>
#include <stdint.h>

/* broadcast / barrier semaphore */

typedef struct {
    primitive_t parent;
    uint32_t tokens;
    uint32_t initial_tokens;
} bsemaphore_t;

void bsemaphore_create_broadcast(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_NOMODIF;                            //initialize a broadcast semaphore with 0 tokens (bsemaphore_flush is required for unlocking)
void bsemaphore_create_barrier(bsemaphore_t *bsem, uint32_t initial_tokens) K_ISR_SAFE K_CSECT_NOMODIF;     //initialize a barrier semaphore, with auto flushing after initial_tokens locks. after flushing semaphore tokens will be reset to initial_tokens
uint32_t bsemaphore_get_token_count(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;              //get bsemaphore token count (0 == locked, otherwise == unlocked)
task_t* bsemaphore_get_waiting_queue(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;           //get bsemaphore waiting_queue (0 == nobody)

void bsemaphore_wait(bsemaphore_t *bsem) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                           //get bsemaphore lock, waiting forever
uint8_t bsemaphore_wait_try(bsemaphore_t *bsem, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;  //get bsemaphore lock, waiting either until you get the lock or until a timeout expires
void bsemaphore_flush(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                            //flushes broadcast semaphore waiting list (unlocks everything), ending in a 0-token semaphore. on a barrier semaphore, flushing indicates urgency in unlocking (after flushing, semaphore tokens will be reset to initial_tokens)

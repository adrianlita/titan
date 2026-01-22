#pragma once

#include <kernel_defines.h>
#include <kernel_atomic.h>
#include <stdint.h>

/* task notifications */
uint32_t notification_get_pending(void) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                      //get pending notifications without clearing them

void notification_send(task_t *task, uint32_t mask) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;      //sets notification bits of task with mask

void notification_set_active(uint32_t mask) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                //sets active notifications based on mask, and clears any previous pending notifications. if mask == 0, disables notifications
uint32_t notification_wait(void) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                           //waits until at least one notification is sent, and returns pending notifications. upon return, clears pending notifications
uint32_t notification_wait_try(uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;           //same as wait, but with timeout. returns 0 when timeout expired, pending notifications otherwise


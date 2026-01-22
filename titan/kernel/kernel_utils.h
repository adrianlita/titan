#pragma once

#include <kernel_defines.h>

/* kernel-aware task priority and sleep queues */
task_t* _kernel_task_queue_prio_add(task_t *queue_head, task_t *task); //add to queue, prio high to low, return queue_head
task_t* _kernel_task_queue_sleep_add(task_t *queue_head, task_t *task, uint32_t tick); //add to queue, timeout low to high, return queue_head
task_t* _kernel_task_queue_prio_remove(task_t *queue_head, task_t *task); //remove element from queue, return queue_head
task_t* _kernel_task_queue_sleep_remove(task_t *queue_head, task_t *task); //remove element from sleep queue, return queue_head
task_t* _kernel_task_queue_peek(task_t *queue_head); //return first element from queue (not modifying it)

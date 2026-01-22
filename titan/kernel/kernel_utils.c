#include "kernel_utils.h"
#include <assert.h>

TITAN_KDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

//add to queue, prio high to low, return queue_head
task_t* _kernel_task_queue_prio_add(task_t *queue_head, task_t *task) {
    //if queue is empty
    if(queue_head == 0) {
        task->prev = 0;
        task->next = 0;
        return task;
    }

    //if queue is lower priority than task
    if(queue_head->prio < task->prio) {
        task->prev = 0;
        task->next = queue_head;
        queue_head->prev = task;
        return task;
    }

    //if queue is higher or equal priority than task
    task_t *p = queue_head;
    while(p->next && (p->next->prio >= task->prio)) {
        p = p->next;
    }

    task->prev = p;
    task->next = p->next;
    if(p->next) {
        p->next->prev = task;
    }
    p->next = task;
    return queue_head;
}

//add to queue, timeout low to high, return queue_head
task_t* _kernel_task_queue_sleep_add(task_t *queue_head, task_t *task, uint32_t tick) {
    //if queue is empty
    if(queue_head == 0) {
        task->sprev = 0;
        task->snext = 0;
        return task;
    }

    task_t *p = queue_head;

    //if the timeout happens before tick overflow
    if(task->timeout > tick) {

        //if queue is higher or equal timeout than task
        if((queue_head->timeout >= task->timeout) || (queue_head->timeout <= tick)) {
            task->sprev = 0;
            task->snext = queue_head;
            queue_head->sprev = task;
            return task;
        }

    }
    else {  //timeout happens after tick overflow
        //we need to move forward until it's passed tick overflow
        while(p->snext && (p->timeout <= p->snext->timeout)) {
            p = p->snext;
        }
    }

    //if queue is higher or equal priority than task
    while(p->snext && (p->snext->timeout < task->timeout)) {
        p = p->snext;
    }

    task->sprev = p;
    task->snext = p->snext;
    if(p->snext) {
        p->snext->sprev = task;
    }
    p->snext = task;
    return queue_head;
}

//remove element from queue, return queue_head
task_t* _kernel_task_queue_prio_remove(task_t *queue_head, task_t *task) {
    task_t *prev = task->prev;
    task_t *next = task->next;

    if(task == queue_head) {
        next->prev = 0;
        queue_head = next;
        return queue_head;
    }

    kassert(prev != 0);
    prev->next = next;
    if(next) {
        next->prev = prev;
    }

    return queue_head;
}

//remove element from queue, return queue_head
task_t* _kernel_task_queue_sleep_remove(task_t *queue_head, task_t *task) {
    task_t *sprev = task->sprev;
    task_t *snext = task->snext;

    if(task == queue_head) {
        snext->sprev = 0;
        queue_head = snext;
        return queue_head;
    }

    kassert(sprev != 0);
    sprev->snext = snext;
    if(snext) {
        snext->sprev = sprev;
    }

    return queue_head;
}

//return first element from queue (not modifying it)
task_t* _kernel_task_queue_peek(task_t *queue_head) {
    return queue_head;
}

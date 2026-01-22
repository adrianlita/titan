#include <pqueue.h>
#include <kernel.h>
#include <assert.h>

TITAN_SKDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

//check if there's someone waiting
static inline uint8_t queue_check_other_end(primitive_t *prim) {
    if(prim->waiting_queue != 0) {
        kernel_current_task->wait_object = (void*)prim;
        kernel_scheduler_task_wait_finished();
        kernel_current_task->wait_object = 0;
        return 1;
    }
    else {
        return 0;
    }
}

void pqueue_create(pqueue_t *q, uint32_t length, void *buffer) K_ISR_FORBID K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(length != 0);
    sassert(buffer != 0);

    KERNEL_CSECT_BEGIN();
    kernel_primitive_create((primitive_t *)q);
    q->parent_send = q->parent_receive;     //also initialize secondary primitive_t
    q->length = length;
    q->buffer = buffer;
    q->count_avail = length;
    q->head = 0;
    q->tail = 0;
    KERNEL_CSECT_END();
}

uint32_t pqueue_get_free(pqueue_t *q) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(q != 0);

    uint32_t result;
    KERNEL_CSECT_BEGIN();
    result = q->count_avail;
    KERNEL_CSECT_END();
    return result;
}


void pqueue_send(pqueue_t *q, const void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    while(q->count_avail == 0) {
        kernel_current_task->wait_object = (void*)&q->parent_send;
        kernel_scheduler_task_wait_start(TS_WAIT_MQ_IN);
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;
    }

    ((uint32_t*)q->buffer)[q->tail] = (uint32_t)message;
    q->tail++;
    if(q->tail >= q->length) {
        q->tail = 0;
    }
    q->count_avail--;

    if(q->parent_receive.waiting_queue != 0) {
        kernel_current_task->wait_object = (void*)&q->parent_receive;
        kernel_scheduler_task_wait_finished();
        kernel_current_task->wait_object = 0;
    }
    kernel_end_critical(&__atomic);
}

uint8_t pqueue_send_try(pqueue_t *q, const void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    uint8_t ret_val = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    kernel_current_task->timeout = _kernel_tick + timeout;
    kernel_current_task->wait_reason = TASK_WAIT_REASON_INIT;
    
    while(q->count_avail == 0) {
        if(timeout == 0) {
            kernel_end_critical(&__atomic);
            return 0;
        }

        kernel_current_task->wait_object = (void*)&q->parent_send;
        kernel_scheduler_task_wait_start((task_state_t)(TS_WAIT_MQ_IN | TS_WAIT_SLEEP));
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting. can be either aquired or expired
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;

        if(kernel_current_task->wait_reason == TASK_WAIT_REASON_SLEEP) {
            break;
        }
    }

    kernel_current_task->timeout = 0;
    if(kernel_current_task->wait_reason & (TASK_WAIT_REASON_INIT | TASK_WAIT_REASON_UNLOCKED)) {
        ret_val = 1;

        ((uint32_t*)q->buffer)[q->tail] = (uint32_t)message;
        q->tail++;
        if(q->tail >= q->length) {
            q->tail = 0;
        }
        q->count_avail--;

        if(queue_check_other_end(&q->parent_receive)) {
            kernel_scheduler_trigger();
        }
    }

    kernel_end_critical(&__atomic);
    return ret_val;
}


void pqueue_send_urgent(pqueue_t *q, const void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    while(q->count_avail == 0) {
        kernel_current_task->wait_object = (void*)&q->parent_send;
        kernel_scheduler_task_wait_start(TS_WAIT_MQ_IN);
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;
    }

    if(q->head == 0) {
        q->head = q->length;
    }
    q->head--;
    ((uint32_t*)q->buffer)[q->head] = (uint32_t)message;
    q->count_avail--;

    if(q->parent_receive.waiting_queue != 0) {
        kernel_current_task->wait_object = (void*)&q->parent_receive;
        kernel_scheduler_task_wait_finished();
        kernel_current_task->wait_object = 0;
    }
    kernel_end_critical(&__atomic);
}

uint8_t pqueue_send_urgent_try(pqueue_t *q, const void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    uint8_t ret_val = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    kernel_current_task->timeout = _kernel_tick + timeout;
    kernel_current_task->wait_reason = TASK_WAIT_REASON_INIT;

    while(q->count_avail == 0) {
        if(timeout == 0) {
            kernel_end_critical(&__atomic);
            return 0;
        }

        kernel_current_task->wait_object = (void*)&q->parent_send;
        kernel_scheduler_task_wait_start((task_state_t)(TS_WAIT_MQ_IN | TS_WAIT_SLEEP));
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting. can be either aquired or expired
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;

        if(kernel_current_task->wait_reason == TASK_WAIT_REASON_SLEEP) {
            break;
        }
    }

    kernel_current_task->timeout = 0;
    if(kernel_current_task->wait_reason & (TASK_WAIT_REASON_INIT | TASK_WAIT_REASON_UNLOCKED)) {
        ret_val = 1;

        if(q->head == 0) {
            q->head = q->length;
        }
        q->head--;
        ((uint32_t*)q->buffer)[q->head] = (uint32_t)message;
        q->count_avail--;

        if(queue_check_other_end(&q->parent_receive)) {
            kernel_scheduler_trigger();
        }
    }

    kernel_end_critical(&__atomic);
    return ret_val;
}

void pqueue_receive(pqueue_t *q, void **message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message != 0);

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    while(q->count_avail == q->length) {
        kernel_current_task->wait_object = (void*)&q->parent_receive;
        kernel_scheduler_task_wait_start(TS_WAIT_MQ_OUT);
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;
    }

    *message = ((uint32_t**)q->buffer)[q->head];
    q->head++;
    if(q->head >= q->length) {
        q->head = 0;
    }
    q->count_avail++;

    if(queue_check_other_end(&q->parent_send)) {
        kernel_scheduler_trigger();
    }
    kernel_end_critical(&__atomic);
}

uint8_t pqueue_receive_try(pqueue_t *q, void **message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message != 0);
    uint8_t ret_val = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    kernel_current_task->timeout = _kernel_tick + timeout;
    kernel_current_task->wait_reason = TASK_WAIT_REASON_INIT;

    while(q->count_avail == q->length) {
        if(timeout == 0) {
            kernel_end_critical(&__atomic);
            return 0;
        }
    
        kernel_current_task->wait_object = (void*)&q->parent_receive;
        kernel_scheduler_task_wait_start((task_state_t)(TS_WAIT_MQ_OUT | TS_WAIT_SLEEP));
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting. can be either aquired or expired
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;

        if(kernel_current_task->wait_reason == TASK_WAIT_REASON_SLEEP) {
            break;
        }
    }

    kernel_current_task->timeout = 0;
    if(kernel_current_task->wait_reason & (TASK_WAIT_REASON_INIT | TASK_WAIT_REASON_UNLOCKED)) {
        ret_val = 1;

        *message = ((uint32_t**)q->buffer)[q->head];
        q->head++;
        if(q->head >= q->length) {
            q->head = 0;
        }
        q->count_avail++;

        if(queue_check_other_end(&q->parent_send)) {
            kernel_scheduler_trigger();
        }
    }

    kernel_end_critical(&__atomic);
    return ret_val;
}

uint8_t pqueue_peek(pqueue_t *q, void **message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message != 0);
    uint8_t result = 0;

    KERNEL_CSECT_BEGIN();
    if(q->count_avail != q->length) {
        *message = ((uint32_t**)q->buffer)[q->head];
        result = 1;
    }
    KERNEL_CSECT_END();
    return result;
}

#include <mqueue.h>
#include <kernel.h>
#include <assert.h>
#include <string.h>

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

void mqueue_create(mqueue_t *q, uint32_t message_size, uint32_t length, void *buffer) K_ISR_FORBID K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message_size != 0);
    sassert(length != 0);
    sassert(buffer != 0);

    KERNEL_CSECT_BEGIN();
    kernel_primitive_create((primitive_t *)q);
    q->parent_send = q->parent_receive;     //also initialize secondary primitive_t
    q->message_size = message_size;
    q->length = length;
    q->buffer = buffer;
    q->count_avail = length;
    q->head = 0;
    q->tail = 0;
    KERNEL_CSECT_END();
}

uint32_t mqueue_get_free(mqueue_t *q) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(q != 0);

    uint32_t result;
    KERNEL_CSECT_BEGIN();
    result = q->count_avail;
    KERNEL_CSECT_END();
    return result;
}


void mqueue_send(mqueue_t *q, const void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message != 0);

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

    memcpy((uint8_t *)q->buffer + (q->tail * q->message_size), message, q->message_size);
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

uint8_t mqueue_send_try(mqueue_t *q, const void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message != 0);
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

        memcpy((uint8_t *)q->buffer + (q->tail * q->message_size), message, q->message_size);
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


void mqueue_send_urgent(mqueue_t *q, const void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message != 0);

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
    memcpy((uint8_t *)q->buffer + (q->head * q->message_size), message, q->message_size);
    q->count_avail--;

    if(q->parent_receive.waiting_queue != 0) {
        kernel_current_task->wait_object = (void*)&q->parent_receive;
        kernel_scheduler_task_wait_finished();
        kernel_current_task->wait_object = 0;
    }
    kernel_end_critical(&__atomic);
}

uint8_t mqueue_send_urgent_try(mqueue_t *q, const void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message != 0);
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
        memcpy((uint8_t *)q->buffer + (q->head * q->message_size), message, q->message_size);
        q->count_avail--;

        if(queue_check_other_end(&q->parent_receive)) {
            kernel_scheduler_trigger();
        }
    }

    kernel_end_critical(&__atomic);
    return ret_val;
}

void mqueue_receive(mqueue_t *q, void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
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

    memcpy(message, (uint8_t *)q->buffer + (q->head * q->message_size), q->message_size);
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

uint8_t mqueue_receive_try(mqueue_t *q, void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
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

        memcpy(message, (uint8_t *)q->buffer + (q->head * q->message_size), q->message_size);
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

uint8_t mqueue_peek(mqueue_t *q, void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(q != 0);
    sassert(message != 0);
    uint8_t result = 0;

    KERNEL_CSECT_BEGIN();
    if(q->count_avail != q->length) {
        memcpy(message, (uint8_t *)q->buffer + (q->head * q->message_size), q->message_size);
        result = 1;
    }
    KERNEL_CSECT_END();
    return result;
}

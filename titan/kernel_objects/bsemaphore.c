#include <bsemaphore.h>
#include <kernel.h>
#include <assert.h>

TITAN_SKDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

void bsemaphore_create_broadcast(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_NOMODIF {
    bsemaphore_create_barrier(bsem, 0);
}

void bsemaphore_create_barrier(bsemaphore_t *bsem, uint32_t initial_tokens) K_ISR_SAFE K_CSECT_NOMODIF {
    sassert(bsem != 0);
    bsem->tokens = initial_tokens;
    bsem->initial_tokens = initial_tokens;
    
    KERNEL_CSECT_BEGIN();
    kernel_primitive_create((primitive_t *)bsem);
    KERNEL_CSECT_END();
}

uint32_t bsemaphore_get_token_count(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(bsem != 0);

    uint32_t result;
    KERNEL_CSECT_BEGIN();
    result = bsem->tokens;
    KERNEL_CSECT_END();
    return result;
}

task_t* bsemaphore_get_waiting_queue(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(bsem != 0);

    task_t *result;
    KERNEL_CSECT_BEGIN();
    result = bsem->parent.waiting_queue;
    KERNEL_CSECT_END();
    return result;
}


static void bsemaphore_internal_flush(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_MUST {
    K_CSECT_MUST_CHECK;

    extern volatile task_t *kernel_current_task;

    bsem->tokens = bsem->initial_tokens;    //reset barrier
    if(bsem->parent.waiting_queue != 0) {
        kernel_current_task->wait_object = (void*)bsem;  //save bsemaphore into wait_object
        while(bsem->parent.waiting_queue != 0) {
            kernel_scheduler_task_wait_finished();
        }
        kernel_current_task->wait_object = 0;
        kernel_scheduler_trigger();
    }
}

void bsemaphore_wait(bsemaphore_t *bsem) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    extern volatile task_t *kernel_current_task;
    sassert(bsem != 0);

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    if(bsem->tokens == 1) {    //it's a barrier semaphore, that will automatically flush right now
        bsemaphore_internal_flush(bsem);
    }
    else {
        if(bsem->tokens) {
            bsem->tokens--;     //it's a barrier semaphore, not yet to be flushed
        }
        
        //either barrier or broadcast, wait for flush to come
        kernel_current_task->wait_object = (void*)bsem;  //save semaphore into wait_object
        kernel_scheduler_task_wait_start(TS_WAIT_SEMA);
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;
    }

    kernel_end_critical(&__atomic);
}

uint8_t bsemaphore_wait_try(bsemaphore_t *bsem, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(bsem != 0);
    uint8_t ret_val = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    if(bsem->tokens == 1) {    //it's a barrier semaphore, that will automatically flush right now
        bsemaphore_internal_flush(bsem);
    }
    else {
        if(timeout == 0) {
            kernel_end_critical(&__atomic);
            return 0;
        }

        if(bsem->tokens) {
            bsem->tokens--;     //it's a barrier semaphore, not yet to be flushed
        }
        
        //either barrier or broadcast, wait for flush to come
        kernel_current_task->wait_reason = TASK_WAIT_REASON_INIT;
        kernel_current_task->timeout = _kernel_tick + timeout;

        kernel_current_task->wait_object = (void*)bsem;  //save semaphore into wait_object
        kernel_scheduler_task_wait_start((task_state_t)(TS_WAIT_SEMA | TS_WAIT_SLEEP));
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;

        if(kernel_current_task->wait_reason == TASK_WAIT_REASON_SLEEP) {
            if(bsem->initial_tokens != 0) {
                //restore token
                bsem->tokens++;
            }
        }
        else { //wait reason is TASK_WAIT_REASON_UNLOCKED
            ret_val = 1;
        }
    }

    kernel_end_critical(&__atomic);
    return ret_val;
}

void bsemaphore_flush(bsemaphore_t *bsem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    extern volatile task_t *kernel_current_task;
    sassert(bsem != 0);

    KERNEL_CSECT_BEGIN();
    bsemaphore_internal_flush(bsem);
    KERNEL_CSECT_END();
}

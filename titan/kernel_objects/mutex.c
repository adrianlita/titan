#include <mutex.h>
#include <kernel.h>
#include <assert.h>

TITAN_SKDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

void mutex_create(mutex_t *mtx) K_ISR_FORBID K_CSECT_NOMODIF {
    sassert(mtx != 0);
    mtx->locks = 0;
    mtx->owner = 0;
    
    KERNEL_CSECT_BEGIN();
    kernel_primitive_create((primitive_t *)mtx);
    KERNEL_CSECT_END();
}

uint32_t mutex_get_locked(mutex_t *mtx) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(mtx != 0);

    uint32_t result;
    KERNEL_CSECT_BEGIN();
    result = mtx->locks;
    KERNEL_CSECT_END();
    return result;
}

task_t* mutex_get_waiting_queue(mutex_t *mtx) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(mtx != 0);

    task_t *result;
    KERNEL_CSECT_BEGIN();
    result = mtx->parent.waiting_queue;
    KERNEL_CSECT_END();
    return result;
}

void mutex_lock(mutex_t *mtx) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    extern volatile task_t *kernel_current_task;
    sassert(mtx != 0);

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    while((mtx->locks != 0) && (mtx->owner != kernel_current_task)) {
        kernel_current_task->wait_object = (void*)mtx;  //save mutex into wait_object
        kernel_scheduler_task_wait_start(TS_WAIT_MUTEX);
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;
    }
    
    mtx->locks++;
    mtx->owner = (task_t *)kernel_current_task;
    kernel_end_critical(&__atomic);
}

uint8_t mutex_lock_try(mutex_t *mtx, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(mtx != 0);
    uint8_t ret_val = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    kernel_current_task->timeout = _kernel_tick + timeout;
    kernel_current_task->wait_reason = TASK_WAIT_REASON_INIT;

    while((mtx->locks != 0) && (mtx->owner != kernel_current_task)) {
        if(timeout == 0) {
            kernel_end_critical(&__atomic);
            return 0;
        }

        kernel_current_task->wait_object = (void*)mtx;  //save mutex into wait_object
        kernel_scheduler_task_wait_start((task_state_t)(TS_WAIT_MUTEX | TS_WAIT_SLEEP));
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
        mtx->locks++;
    }

    kernel_end_critical(&__atomic);
    return ret_val;
}

void mutex_unlock(mutex_t *mtx) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    extern volatile task_t *kernel_current_task;
    sassert(mtx != 0);

    KERNEL_CSECT_BEGIN();
    if((mtx->owner == kernel_current_task) && (mtx->locks > 0)) {        

        mtx->locks--;
        if(mtx->locks == 0) {
            mtx->owner = 0;
            if(mtx->parent.waiting_queue != 0) {
                kernel_current_task->wait_object = (void*)mtx;  //save mutex into wait_object
                kernel_scheduler_task_wait_finished();
                kernel_current_task->wait_object = 0;
                kernel_scheduler_trigger();
            }
        }

    }
    KERNEL_CSECT_END();
}

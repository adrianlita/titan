#include <semaphore.h>
#include <kernel.h>
#include <assert.h>

TITAN_SKDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

void semaphore_create(semaphore_t *sem, uint32_t initial_tokens) K_ISR_SAFE K_CSECT_NOMODIF {
    sassert(sem != 0);
    sem->tokens = initial_tokens;
    
    KERNEL_CSECT_BEGIN();
    kernel_primitive_create((primitive_t *)sem);
    KERNEL_CSECT_END();
}

uint32_t semaphore_get_token_count(semaphore_t *sem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(sem != 0);

    uint32_t result;
    KERNEL_CSECT_BEGIN();
    result = sem->tokens;
    KERNEL_CSECT_END();
    return result;
}

task_t* semaphore_get_waiting_queue(semaphore_t *sem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(sem != 0);

    task_t *result;
    KERNEL_CSECT_BEGIN();
    result = sem->parent.waiting_queue;
    KERNEL_CSECT_END();
    return result;
}

void semaphore_lock(semaphore_t *sem) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    extern volatile task_t *kernel_current_task;
    sassert(sem != 0);

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    while(sem->tokens == 0) {
        kernel_current_task->wait_object = (void*)sem;  //save semaphore into wait_object
        kernel_scheduler_task_wait_start(TS_WAIT_SEMA);
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;
    }

    sem->tokens--;
    kernel_end_critical(&__atomic);
}

uint8_t semaphore_lock_try(semaphore_t *sem, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(sem != 0);
    uint8_t ret_val = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    kernel_current_task->timeout = _kernel_tick + timeout;
    kernel_current_task->wait_reason = TASK_WAIT_REASON_INIT;

    while(sem->tokens == 0) {
        if(timeout == 0) {
            kernel_end_critical(&__atomic);
            return 0;
        }

        kernel_current_task->wait_object = (void*)sem;  //save semaphore into wait_object
        kernel_scheduler_task_wait_start((task_state_t)(TS_WAIT_SEMA | TS_WAIT_SLEEP));
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
        sem->tokens--;
    }

    kernel_end_critical(&__atomic);
    return ret_val;
}

void semaphore_unlock(semaphore_t *sem) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    extern volatile task_t *kernel_current_task;
    sassert(sem != 0);

    KERNEL_CSECT_BEGIN();
    sem->tokens++;
    if(sem->parent.waiting_queue != 0) {
        kernel_current_task->wait_object = (void*)sem;  //save semaphore into wait_object
        kernel_scheduler_task_wait_finished();
        kernel_current_task->wait_object = 0;
        kernel_scheduler_trigger();
    }
    KERNEL_CSECT_END();
}

void semaphore_multiunlock(semaphore_t *sem, uint32_t tokens) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    extern volatile task_t *kernel_current_task;
    sassert(sem != 0);

    if(tokens == 0) {
        return;
    }

    KERNEL_CSECT_BEGIN();
    sem->tokens += tokens;
    if(sem->parent.waiting_queue != 0) {
        kernel_current_task->wait_object = (void*)sem;  //save semaphore into wait_object
        while((tokens != 0) && (sem->parent.waiting_queue != 0)) {
            tokens--;
            kernel_scheduler_task_wait_finished();
        }
        kernel_current_task->wait_object = 0;
        kernel_scheduler_trigger();
    }
    KERNEL_CSECT_END();
}

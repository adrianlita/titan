#include <notification.h>
#include <kernel.h>
#include <assert.h>

TITAN_SKDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

uint32_t notification_get_pending(void) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    uint32_t result;

    KERNEL_CSECT_BEGIN();
    result = kernel_current_task->notif_pending;
    KERNEL_CSECT_END();

    return result;
}

void notification_send(task_t *task, uint32_t mask) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);
    sassert(mask != 0);

    KERNEL_CSECT_BEGIN();
    kernel_scheduler_task_notify(task, mask);
    KERNEL_CSECT_END();
}

void notification_set_active(uint32_t mask) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    KERNEL_CSECT_BEGIN();
    kernel_current_task->notif_pending = 0;
    kernel_current_task->notif_active = mask;
    KERNEL_CSECT_END();
}

uint32_t notification_wait(void) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    uint32_t pending;
    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    while(kernel_current_task->notif_pending == 0) {
        kernel_scheduler_task_wait_start(TS_WAIT_NOTIFY);
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);
    }
    pending = kernel_current_task->notif_pending;
    kernel_current_task->notif_pending = 0;
    kernel_end_critical(&__atomic);
    return pending;
}

uint32_t notification_wait_try(uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;
    
    uint32_t pending;
    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    kernel_current_task->timeout = _kernel_tick + timeout;
    kernel_current_task->wait_reason = TASK_WAIT_REASON_INIT;

    while(kernel_current_task->notif_pending == 0) {
        if(timeout == 0) {
            kernel_end_critical(&__atomic);
            return 0;
        }

        kernel_scheduler_task_wait_start((task_state_t)(TS_WAIT_NOTIFY | TS_WAIT_SLEEP));
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //here comes back from waiting
        kernel_begin_critical(&__atomic);

        if(kernel_current_task->wait_reason == TASK_WAIT_REASON_SLEEP) {
            pending = 0;
            break;
        }
    }

    kernel_current_task->timeout = 0;
    if(kernel_current_task->wait_reason & (TASK_WAIT_REASON_INIT | TASK_WAIT_REASON_UNLOCKED)) {
        pending = kernel_current_task->notif_pending;
        kernel_current_task->notif_pending = 0;
    }

    kernel_end_critical(&__atomic);
    return pending;
}

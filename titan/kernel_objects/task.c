#include <task.h>
#include <titan_defs.h>
#include <kernel.h>
#include <kernel_utils.h>
#include <assert.h>

TITAN_SKDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

#define PREPARE_FLAG_CREATE         0x00
#define PREPARE_FLAG_START          0x01
#define PREPARE_FLAG_RESTART        0x02

static void task_prepare_stack_and_reset_pointers(task_t *task, void *arg, uint8_t prepare_flag) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    task_stack_t *stack_pointer = (task_stack_t *)((uint32_t)task->stack_min + task->stack_size * sizeof(task_stack_t));

    //populate initial stack for debugging purposes
    for(task_stack_t *i = task->stack_min; i != stack_pointer; i++) {
        *i = TASK_STACK_INIT_VALUE;
    }

    //populate task's stack with dummy values
    *(--stack_pointer) = (1U << 24);                        // xPSR
    *(--stack_pointer) = (uint32_t)task->start_routine;   // PC
    *(--stack_pointer) = (uint32_t)task_exit_r;           // LR  kernel_scheduler_task_exit checkAL verifica inca o data care-i treaba si poate facem altfel
    *(--stack_pointer) = 0x0U;                              // R12
    *(--stack_pointer) = 0x0U;                              // R3 
    *(--stack_pointer) = 0x0U;                              // R2 
    *(--stack_pointer) = 0x0U;                              // R1 
    *(--stack_pointer) = (task_stack_t)arg;               // R0 - pass argument
    #if (__FPU_USED == 1)
    *(--stack_pointer) = EXC_RETURN_THREAD_PSP;             // EXC_RETURN
    #endif
    *(--stack_pointer) = 0x0U;                              // R11
    *(--stack_pointer) = 0x0U;                              // R10
    *(--stack_pointer) = 0x0U;                              // R9
    *(--stack_pointer) = 0x0U;                              // R8
    *(--stack_pointer) = 0x0U;                              // R7
    *(--stack_pointer) = 0x0U;                              // R6
    *(--stack_pointer) = 0x0U;                              // R5
    *(--stack_pointer) = 0x0U;                              // R4


    task->stack_pointer = (void*)stack_pointer;

    task->prev = 0;
    task->next = 0;
    task->sprev = 0;
    task->snext = 0;

    if(prepare_flag != PREPARE_FLAG_START) {
        task->join_head = 0;              //list of tasks waiting to join
    }

    task->wait_object = 0;            //not waiting on primitives
    task->wait_reason = 0;
}

void task_init(task_t *task, task_stack_t *stack, uint32_t stack_size, uint8_t priority) K_ISR_SAFE K_CSECT_NOMODIF {
    sassert(task != 0);
    sassert(stack != 0);
    sassert(stack_size >= TASK_STACK_SIZE_MIN);
    sassert(((((uint32_t)stack + stack_size) / 8) * 8) == (((uint32_t)stack + stack_size)));    //stack must be 8-byte aligned to conform to the AAPCS

    task->key = TASK_KEY_VALUE;
    task->stack_min = (void*)stack;
    task->stack_size = stack_size;
    task->prio = priority;

    task->start_type = (task_start_type_t)0;
    task->timeout = 0;
}

void task_attr_set_start_type(task_t *task, task_start_type_t start_type, uint32_t timeout) K_ISR_SAFE K_CSECT_NOMODIF {
    sassert(task != 0);
    sassert(task->key == TASK_KEY_VALUE);

    task->start_type = start_type;
    task->timeout = timeout;
}

void task_create(task_t *task, void(*start_routine)(void)) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    task_create_ra(task, (task_routine_t)start_routine, 0);
}

void task_create_a(task_t *task, void(*start_routine)(void*), void *arg) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    task_create_ra(task, (task_routine_t)start_routine, arg);
}

void task_create_ra(task_t *task, void*(*start_routine)(void*), void *arg) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);
    sassert(task->key == TASK_KEY_VALUE);
    sassert(start_routine != 0);

    KERNEL_CSECT_BEGIN();
    task->start_routine = start_routine;
    task_prepare_stack_and_reset_pointers(task, arg, PREPARE_FLAG_CREATE);

    switch(task->start_type) {
        case TASK_START_NOW:
            task->state = TS_READY;
            task->timeout = 0;
            break;

        case TASK_START_MANUAL:
            task->state = TS_STOPPED;
            task->timeout = 0;
            break;

        case TASK_START_DELAYED:
            task->state = TS_WAIT_SLEEP;
            break;

        default:
            sassert(0);
            break;
    }
    
    //thead is ready, add it to the kernel list and return
    kernel_task_create(task);
    kernel_scheduler_task_start(task, task->state);
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

void task_start(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);
    sassert(task->id != 0);

    KERNEL_CSECT_BEGIN();
    sassert((task->state & TS_NOT_STARTED) != 0);
    task_prepare_stack_and_reset_pointers(task, 0, PREPARE_FLAG_START);
    kernel_scheduler_task_start(task, TS_READY);
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

void task_start_a(task_t *task, void *arg) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);
    sassert(task->id != 0);

    KERNEL_CSECT_BEGIN();
    sassert((task->state & TS_NOT_STARTED) != 0);
    task_prepare_stack_and_reset_pointers(task, arg, PREPARE_FLAG_START);
    kernel_scheduler_task_start(task, TS_READY);
    KERNEL_CSECT_END();
}

void task_exit(void) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    KERNEL_CSECT_BEGIN();
    kernel_scheduler_task_kill((task_t *)kernel_current_task, 0);
    kernel_current_task->state = TS_EXITED;
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

void task_exit_r(void *retval) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    KERNEL_CSECT_BEGIN();
    kernel_scheduler_task_kill((task_t *)kernel_current_task, retval);
    kernel_current_task->state = TS_EXITED;
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

void task_kill(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);
    sassert(task->id != 0);

    KERNEL_CSECT_BEGIN();
    kernel_scheduler_task_kill(task, 0);
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

void task_kill_r(task_t *task, void *ret) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);
    sassert(task->id != 0);

    KERNEL_CSECT_BEGIN();
    kernel_scheduler_task_kill(task, ret);
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

uint8_t task_ready(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = ((task->state & (TS_READY | TS_RUNNING)) != 0);
    KERNEL_CSECT_END();
    return result;
}

uint8_t task_started(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = ((task->state & TS_STARTED) != 0);
    KERNEL_CSECT_END();
    return result;
}

uint8_t task_waiting(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = ((task->state & TS_WAITING) != 0);
    KERNEL_CSECT_END();
    return result;
}

uint8_t task_sleeping(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = ((task->state & TS_WAIT_SLEEP) != 0);
    KERNEL_CSECT_END();
    return result;
}

uint8_t task_stopped(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = ((task->state & TS_STOPPED) != 0);
    KERNEL_CSECT_END();
    return result;
}

uint8_t task_exited(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = ((task->state & TS_EXITED) != 0);
    KERNEL_CSECT_END();
    return result;
}

uint8_t task_killed(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = ((task->state & TS_KILLED) != 0);
    KERNEL_CSECT_END();
    return result;
}

uint8_t task_finished(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    sassert(task != 0);

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = ((task->state & TS_FINISHED) != 0);
    KERNEL_CSECT_END();
    return result;
}

uint8_t task_priority_get(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    if(task == 0) {
        task = (task_t *)kernel_current_task;
    }

    uint8_t result;
    KERNEL_CSECT_BEGIN();
    result = task->prio;
    KERNEL_CSECT_END();
    return result;
}

void task_priority_set(task_t *task, uint8_t priority) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    if(task == 0) {
        task = (task_t *)kernel_current_task;
    }

    KERNEL_CSECT_BEGIN();
    task->prio = priority;
    KERNEL_CSECT_END();
}

void task_priority_set_immediate(task_t *task, uint8_t priority) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    if(task == 0) {
        task = (task_t *)kernel_current_task;
    }

    KERNEL_CSECT_BEGIN();
    kernel_scheduler_task_change_priority(task, priority);
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}


task_t *task_self(void) K_ISR_SAFE K_CSECT_NOMODIF {
    return (task_t *)kernel_current_task;
}

void task_yield(void) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    KERNEL_CSECT_BEGIN();
    kernel_scheduler_yield();
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

void task_join(task_t *task, void **retval) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(task != 0);

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    if((task->state & TS_FINISHED) != 0) {
        if(retval) {
            *retval = task->retval;
        }
        kernel_end_critical(&__atomic);
        return;
    }

    while((task->state & TS_FINISHED) == 0) {
        kernel_current_task->wait_object = (void*)task;
        kernel_scheduler_task_wait_start(TS_WAIT_JOIN);
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //by here we're out of waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;
    }

    if(retval) {
        *retval = task->retval;
    }
    kernel_end_critical(&__atomic);
}

uint8_t task_join_try(task_t *task, void **retval, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(task != 0);
    uint8_t join_retval = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);

    kernel_current_task->timeout = _kernel_tick + timeout;
    kernel_current_task->wait_reason = TASK_WAIT_REASON_INIT;

    while((task->state & TS_FINISHED) == 0) {
        if(timeout == 0) {
            kernel_end_critical(&__atomic);
            return 0;
        }

        kernel_current_task->wait_object = (void*)task;
        kernel_scheduler_task_wait_start((task_state_t)(TS_WAIT_JOIN | TS_WAIT_SLEEP));
        kernel_scheduler_trigger();
        kernel_end_critical(&__atomic);
        //by here we're out of waiting
        kernel_begin_critical(&__atomic);
        kernel_current_task->wait_object = 0;

        if(kernel_current_task->wait_reason == TASK_WAIT_REASON_SLEEP) {
            break;
        }
    }

    kernel_current_task->timeout = 0;
    if(kernel_current_task->wait_reason & (TASK_WAIT_REASON_INIT | TASK_WAIT_REASON_UNLOCKED)) {
        if(retval) {
            *retval = task->retval;
        }
        join_retval = 1;
    }

    kernel_end_critical(&__atomic);
    return join_retval;
}

void task_sleep(uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(kernel_current_task->id != 0); //idle task must not sleep

    if(timeout == 0) {
        return;
    }

    KERNEL_CSECT_BEGIN();
    kernel_current_task->timeout = _kernel_tick + timeout;
    kernel_scheduler_task_sleep_start();
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

void task_sleep_periodic(uint32_t *timer_buffer, uint32_t period) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;

    sassert(kernel_current_task->id != 0); //idle task must not sleep
    sassert(timer_buffer != 0);

    if(period == 0) {
        *timer_buffer = _kernel_tick;
        return;
    }

    KERNEL_CSECT_BEGIN();
    *timer_buffer += period;
    if(*timer_buffer <= _kernel_tick) {
        *timer_buffer = _kernel_tick + 1;
    }
    kernel_current_task->timeout = *timer_buffer;
    kernel_scheduler_task_sleep_start();
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

void task_wake(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    uint8_t sleeping;

    sleeping = ((task->state & TS_WAIT_SLEEP) != 0);
    if(!sleeping) {
        return;
    }

    KERNEL_CSECT_BEGIN();
    kernel_scheduler_task_sleep_finished(task);
    kernel_scheduler_trigger();
    KERNEL_CSECT_END();
}

uint32_t task_info_stack_get_total(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    uint32_t result;
    KERNEL_CSECT_BEGIN();
    if(task == 0) {
        task = (task_t *)kernel_current_task;
    }

    result = task->stack_size;
    KERNEL_CSECT_END();
    return result;
}

uint32_t task_info_stack_get_used(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    uint32_t result;
    KERNEL_CSECT_BEGIN();
    if(task == 0) {
        task = (task_t *)kernel_current_task;
    }

    result = task->stack_size - (((uint32_t)task->stack_pointer - (uint32_t)task->stack_min) / sizeof(task_stack_t));
    KERNEL_CSECT_END();
    return result;
}

uint32_t task_info_stack_get_maxused(task_t *task) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT {
    uint32_t result;
    task_stack_t *last_entry;
    KERNEL_CSECT_BEGIN();
    if(task == 0) {
        task = (task_t *)kernel_current_task;
    }

    last_entry = task->stack_min;
    while(*last_entry == TASK_STACK_INIT_VALUE) {
        last_entry++;
    }

    result = task->stack_size - (((uint32_t)last_entry - (uint32_t)task->stack_min) / sizeof(task_stack_t));
    KERNEL_CSECT_END();
    return result;
}

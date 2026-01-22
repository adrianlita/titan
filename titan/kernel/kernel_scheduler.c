#include <kernel.h>
#include <kernel_utils.h>
#include <kernel_supervisor.h>
#include <assert.h>

TITAN_KDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

/* first 2 are used by ASM context switch interrupt */
volatile task_t *kernel_current_task;                         //holds current running task
volatile task_t *kernel_ready_queue;                          //head of the "TS_READY" tasks, priority ordered. also, the next schedulable task
volatile task_t *kernel_sleep_queue;                          //head of the "TS_WAIT_SLEEP" tasks, wake-time ordered. also includes tasks waiting for other objects, but with timeout


void kernel_scheduler_init(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    kernel_current_task = 0;
    kernel_ready_queue = 0;
    kernel_sleep_queue = 0;
}

__NO_RETURN __STACKLESS void kernel_scheduler_start(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_EXIT {
    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    //get first runnable task and mark it as running, then switch context to it
    kernel_current_task = _kernel_task_queue_peek((task_t *)kernel_ready_queue);
    kernel_current_task->state = TS_RUNNING;

    __asm volatile (
        "MRS    R0, MSP\n"          //load main stack pointer into R0
        "SUB    R0, R0, #0x20\n"    //substract the values automatically pushed by SVC_Handler
        "MSR    PSP, R0\n"          //save value into PSP, as it will be used by the SVC_Handler later
        "CPSIE  I\n"                //enable interrupts so SVC can run
        "SVC    #"STRINGIFY(KERNEL_SVC_FIRST_RUN)"\n"            //call SVC with parameter for "first run"
    );
    while(1);   //make sure the compiler treats function as a __NO_RETURN
}

void kernel_scheduler_trigger(void) K_ISR_SAFE K_CSECT_NOMODIF {
    if(kernel_current_task != kernel_ready_queue) {
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}

void kernel_scheduler_yield(void) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    //reschedule current task
    kernel_ready_queue = _kernel_task_queue_prio_remove((task_t *)kernel_ready_queue, (task_t *)kernel_current_task);
    kernel_ready_queue = _kernel_task_queue_prio_add((task_t *)kernel_ready_queue, (task_t *)kernel_current_task);
}

void kernel_scheduler_task_start(task_t *task, task_state_t new_state) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    kassert(task != 0);
    
    task->state = new_state;
    switch(new_state) {
        case TS_READY:
            kernel_ready_queue = _kernel_task_queue_prio_add((task_t *)kernel_ready_queue, task);
            break;

        case TS_WAIT_SLEEP:
            kernel_sleep_queue = _kernel_task_queue_sleep_add((task_t *)kernel_sleep_queue, task, _kernel_tick);
            break;

        case TS_STOPPED:
            break;

        default:
            kassert(0);
            break;
    }
}

void kernel_scheduler_task_kill(task_t *task, void *retval) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    kassert(task != 0);

    /* check state and take task out of its respective queues */
    if((task->state & (TS_RUNNING | TS_READY)) != 0) {
        kernel_ready_queue = _kernel_task_queue_prio_remove((task_t *)kernel_ready_queue, (task_t *)task);
    }

    if((task->state & TS_WAIT_SLEEP) != 0) {
        kernel_sleep_queue = _kernel_task_queue_sleep_remove((task_t *)kernel_sleep_queue, task);
    }

    if((task->state & TS_WAIT_JOIN) != 0) {
        task_t *join_task = (task_t *)(kernel_current_task->wait_object);  //retrieve join task
        join_task->join_head = _kernel_task_queue_prio_add(join_task->join_head, task);
    }

    if((task->state & TS_WAITING_PRIM) != 0) {
        primitive_t *primitive = (primitive_t *)(task->wait_object);   //object
        primitive->waiting_queue = _kernel_task_queue_prio_remove(primitive->waiting_queue, task);
    }

    //futureAL[1]

    /* check if task has other tasks waiting to join */
    while(task->join_head != 0) {
        task_t *waiting_task = _kernel_task_queue_peek((task_t *)task->join_head);
        task->join_head = _kernel_task_queue_prio_remove((task_t *)task->join_head, waiting_task);
        kernel_ready_queue = _kernel_task_queue_prio_add((task_t *)kernel_ready_queue, waiting_task);

        //check wheteher it's in the sleep queue as well
        if(waiting_task->state & TS_WAIT_SLEEP) {
            kernel_sleep_queue = _kernel_task_queue_sleep_remove((task_t *)kernel_sleep_queue, waiting_task);
            waiting_task->timeout = 0;
        }

        waiting_task->wait_reason = TASK_WAIT_REASON_UNLOCKED;
        waiting_task->state = TS_READY;
    }

    task->retval = retval;
    task->state = TS_KILLED;
}

void kernel_scheduler_task_change_priority(task_t *task, uint8_t priority) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    kassert(task != 0);

    /* check state and take task out of its respective queues */
    
    //sleep queue isn't affected by priority
    //ts_finished / destroyable are not affected
    task->prio = priority;

    if((task->state & (TS_RUNNING | TS_READY)) != 0) {
        kernel_ready_queue = _kernel_task_queue_prio_remove((task_t *)kernel_ready_queue, (task_t *)task);
        kernel_ready_queue = _kernel_task_queue_prio_add((task_t *)kernel_ready_queue, (task_t *)task);
    }

    if((task->state & TS_WAIT_JOIN) != 0) {
        task_t *join_task = (task_t *)(kernel_current_task->wait_object);  //retrieve join task
        join_task->join_head = _kernel_task_queue_prio_remove(join_task->join_head, task);
        join_task->join_head = _kernel_task_queue_prio_add(join_task->join_head, task);
    }

    if((task->state & TS_WAITING_PRIM) != 0) {
        primitive_t *primitive = (primitive_t *)(task->wait_object);   //object
        primitive->waiting_queue = _kernel_task_queue_prio_remove(primitive->waiting_queue, task);
        primitive->waiting_queue = _kernel_task_queue_prio_add(primitive->waiting_queue, task);
    }
}


void kernel_scheduler_task_sleep_start(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    kassert(kernel_current_task->state == TS_RUNNING);

    kernel_ready_queue = _kernel_task_queue_prio_remove((task_t *)kernel_ready_queue, (task_t *)kernel_current_task);
    kernel_sleep_queue = _kernel_task_queue_sleep_add((task_t *)kernel_sleep_queue, (task_t *)kernel_current_task, _kernel_tick);
    kernel_current_task->state = TS_WAIT_SLEEP;
}

void kernel_scheduler_task_sleep_finished(task_t *task) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    kassert(task != 0);

    kernel_sleep_queue = _kernel_task_queue_sleep_remove((task_t *)kernel_sleep_queue, task);
    
    //check whether the task is also in another waiting queue
    task->state -= TS_WAIT_SLEEP;
    if(task->state != 0) {
        switch(task->state) {
            case TS_WAIT_JOIN: {
                task_t *waiting_task = (task_t *)task->wait_object;
                waiting_task->join_head = _kernel_task_queue_prio_remove(waiting_task->join_head, task);
                break;
            }

            case TS_WAIT_SEMA: 
            case TS_WAIT_MUTEX:
            case TS_WAIT_MQ_IN:
            case TS_WAIT_MQ_OUT:
            {
                primitive_t *primitive = (primitive_t *)(task->wait_object);   //object
                primitive->waiting_queue = _kernel_task_queue_prio_remove(primitive->waiting_queue, task);
                break;
            }

            case TS_WAIT_NOTIFY:
                //nothing else to do here
                break;

            default:
                kassert(0);
                break;

        }

        task->wait_reason = TASK_WAIT_REASON_SLEEP;
    }

    task->timeout = 0;
    task->state = TS_READY;
    kernel_ready_queue = _kernel_task_queue_prio_add((task_t *)kernel_ready_queue, task);
}


void kernel_scheduler_task_wait_start(task_state_t new_state) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    kassert((new_state & TS_WAITING) != 0);

    kernel_current_task->state = new_state;

    kernel_ready_queue = _kernel_task_queue_prio_remove((task_t *)kernel_ready_queue, (task_t *)kernel_current_task);
    if(new_state & TS_WAIT_SLEEP) {
        kernel_sleep_queue = _kernel_task_queue_sleep_add((task_t *)kernel_sleep_queue, (task_t *)kernel_current_task, _kernel_tick);
        new_state -= TS_WAIT_SLEEP;
    }

    switch(new_state) {
        case TS_WAIT_JOIN: {  //task is blocking on a task join
            task_t *join_task = (task_t *)(kernel_current_task->wait_object);  //retrieve join task
            join_task->join_head = _kernel_task_queue_prio_add(join_task->join_head, (task_t *)kernel_current_task);
            break;
        }

        case TS_WAIT_SEMA: 
        case TS_WAIT_MUTEX:
        case TS_WAIT_MQ_IN:
        case TS_WAIT_MQ_OUT:
        {  //task is blocking on primitive
            primitive_t *primitive = (primitive_t *)(kernel_current_task->wait_object);   //object
            primitive->waiting_queue = _kernel_task_queue_prio_add(primitive->waiting_queue, (task_t *)kernel_current_task);
            break;
        }

        case TS_WAIT_NOTIFY:
            //don't put it in any other queue
            break;

        default:
            kassert(0);
            break;
    }

}

void kernel_scheduler_task_wait_finished(void) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    primitive_t *primitive = (primitive_t *)(kernel_current_task->wait_object);   //object
    task_t *task = _kernel_task_queue_peek(primitive->waiting_queue);
    primitive->waiting_queue = _kernel_task_queue_prio_remove(primitive->waiting_queue, task);
    kernel_ready_queue = _kernel_task_queue_prio_add((task_t *)kernel_ready_queue, task);
    
    //if exists, remove it also from the sleeping queue
    if(task->state & TS_WAIT_SLEEP) {
        kernel_sleep_queue = _kernel_task_queue_sleep_remove((task_t *)kernel_sleep_queue, task);
    }

    task->wait_reason = TASK_WAIT_REASON_UNLOCKED;
    task->state = TS_READY;
}

void kernel_scheduler_task_notify(task_t *task, uint32_t mask) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    kassert(task != 0);

    //check mask, check etc
    if((task->notif_active & mask) != 0) {    //if at least one bit is set
        task->notif_pending |= (task->notif_active & mask); //set the active bits

        if((task->state & TS_WAIT_NOTIFY) != 0) {
            kernel_ready_queue = _kernel_task_queue_prio_add((task_t *)kernel_ready_queue, task);
            
            //if exists, remove it also from the sleeping queue
            if(task->state & TS_WAIT_SLEEP) {
                kernel_sleep_queue = _kernel_task_queue_sleep_remove((task_t *)kernel_sleep_queue, task);
            }

            task->wait_reason = TASK_WAIT_REASON_UNLOCKED;
            task->state = TS_READY;
        }
    }
}

void kernel_scheduler_io_wait_start(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    kassert(kernel_current_task->state == TS_RUNNING);

    kernel_ready_queue = _kernel_task_queue_prio_remove((task_t *)kernel_ready_queue, (task_t *)kernel_current_task);
    kernel_current_task->state = TS_WAIT_IO;
}

void kernel_scheduler_io_wait_finished(task_t *task) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK; //checkAL --- asta e chemata exclsuiv pe ISR. trebuie sa bagam treaba cu kernel-aware-interrupts, si trebuie sa bagam asta si la SysTick

    kernel_ready_queue = _kernel_task_queue_prio_add((task_t *)kernel_ready_queue, task);
    task->state = TS_READY;
}

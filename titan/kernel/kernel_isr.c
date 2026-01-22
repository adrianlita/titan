#include <kernel.h>
#include <kernel_utils.h>
#include <kernel_supervisor.h>
#include <titan_defs.h>
#include <assert.h>

TITAN_KDEBUG_FILE_MARK;

volatile uint32_t _kernel_tick;

extern volatile task_t *kernel_current_task;
extern volatile task_t *kernel_ready_queue;
extern volatile task_t *kernel_sleep_queue;

#define SCB_VTOR    0xE000ED08  //(SCB->VTOR)
#define SCB_CPACR   0xE000ED88  //(SCB->CPACR)

void SysTick_Handler(void) {
    _kernel_tick++;         //increase tick

    //check for delayed tasks if ready
    while((kernel_sleep_queue != 0) && (kernel_sleep_queue->timeout == _kernel_tick)) {
        task_t *task = _kernel_task_queue_peek((task_t *)kernel_sleep_queue);
        kernel_scheduler_task_sleep_finished(task);
    }

    #ifndef TITAN_KERNEL_TICKLESS_OPERATION
    kernel_scheduler_yield();  //force task to yield to do the round-robin
    #endif

    kernel_scheduler_trigger();
}

__STACKLESS void kernel_scheduler_first_run_asm(void) {
    __asm volatile (
        "LDR    R0, =kernel_current_task\n"                 // load current task address into R0
        "LDR    R0, [R0]\n"                                 // load into R0 current stack pointer (also start of TCB)        
        "LDR    R2, [R0]\n"                                 // load new task PSP
        "LDMIA  R2!, {R4-R11}\n"                            // pop R4-R11 onto PSP
    #if (__FPU_USED == 1)
        "LDMIA  R2!, {LR}\n"                                // pop LR onto PSP
    #else
        "MOV    LR, #"STRINGIFY(EXC_RETURN_THREAD_PSP)"\n"  // load new EXC_RETURN_THREAD_PSP
    #endif
    
        "MSR    PSP, R2\n"                                  // save PSP

        "LDR    R0, ="STRINGIFY(SCB_VTOR)"\n"               // load VTOR address which stores main stack base
        "LDR    R0, [R0]\n"                                 // load VTOR
        "LDR    R0, [R0]\n"                                 // load MSP base
        "MSR    MSP, R0\n"                                  // save address (empty stack) to the main stack pointer
        "BX     LR\n"                                       // return to current_task
        //registers R0-R3, R12, LR, PC and xPSR will automatically POP from PSP after BX LR
    );
}

__STACKLESS void SVC_Handler_C(uint32_t *param) {
    uint32_t PC_val;
    uint8_t SVC_number;

    PC_val = param[6];
    SVC_number = ((uint8_t*)PC_val)[-2];

    switch(SVC_number) {
        case KERNEL_SVC_FIRST_RUN:
            __asm("B    kernel_scheduler_first_run_asm\n");
            break;

        case KERNEL_SVC_DISABLE_FPU:
            SCB->CPACR &= ~((3UL << 10*2)|(3UL << 11*2));   //set CP10,11 to access denied
            __asm volatile("ORR LR, LR, #0x10");            //set return from SVC bit to no-FP
            break;

        default:
            kassert(0);
            break;
    }
}

__STACKLESS void SVC_Handler(void) {
    __asm volatile (
        "MRS    R0, PSP\n"              // save process stack pointer into R0
        "B      SVC_Handler_C\n"        // go to C-version of the SVC_Handler
    );
}

__STACKLESS void PendSV_Handler(void) {
    __asm volatile (
        //automatically pushes R0-R3, R12, LR, PC, PSR onto PSP
        "CPSID  I\n"                                        // disable interrupts, critical section begin

        "MRS    R2, PSP\n"                                  // load PSP into R2
    #if (__FPU_USED == 1)
        #if (KERNEL_FPU_ALWAYS_SAVING_CONTEXT == 1)
        "TST    LR, #0x10\n"                                // check if FPCA was used by the task
	    "IT     EQ\n"
        "VSTMDBEQ R2!, {S16-S31}\n"                         // store the rest of the registers. this is also where the lazy stacking occurs
        #endif

        "STMDB  R2!, {LR}\n"                                // push LR onto PSP
    #endif
        "STMDB  R2!, {R4-R11}\n"                            // push R4-R11 onto PSP
        "LDR    R0, =kernel_current_task\n"                 // load current task address into R0
        "LDR    R1, [R0]\n"                                 // load current task stack pointer (also start of TCB)
        "STR    R2, [R1]\n"                                 // store new stack pointer for current task
        // if state is running, change it to ready
        "LDR    R2, [R1, #4]\n"                             // load task_state (TCB_start + 4)
        "TEQ    R2, #"STRINGIFY(_TS_RUNNING_)"\n"           // test if state == TS_RUNNING
        "IT     EQ\n"                                       // if-then for the next instruction
        "MOVEQ  R2, #"STRINGIFY(_TS_READY_)"\n"             // if state == TS_RUNNING, put R2 == TS_READY
        "STR    R2, [R1, #4]\n"                             // store new state (second then)

        "LDR    R1, =kernel_ready_queue\n"                  // load new task address into R0
        "LDR    R1, [R1]\n"                                 // load new task stack pointer (also start of TCB)
        "LDR    R2, [R1, #4]\n"                             // load task_state (TCB_start + 4)
        "MOV    R2, #"STRINGIFY(_TS_RUNNING_)"\n"           // set it to TS_RUNNING
        "STR    R2, [R1, #4]\n"                             // store new state

        "LDR    R2, [R1]\n"                                 // load new task PSP
        "LDMIA  R2!, {R4-R11}\n"                            // pop R4-R11 onto PSP
    #if (__FPU_USED == 1)
        "LDMIA  R2!, {LR}\n"                                // pop LR onto PSP
        #if (KERNEL_FPU_ALWAYS_SAVING_CONTEXT == 1)
        "TST    LR, #0x10\n"                                // check if FPCA was used by the task
        "IT     EQ\n"
        "VLDMIAEQ R2!, {S16-S31}\n"                         // also load FPU registers if task was running in FPU context
        #endif
    #endif
        "MSR    PSP, R2\n"                                  // save PSP
        "STR    R1, [R0]\n"                                 // kernel_current_task = _kernel_ready_queue
        "CPSIE  I\n"                                        // enable interrupts again, critical section end
        "BX     LR\n"                                       // branch to the switched task
    );
}

__STACKLESS void HardFault_Handler(void) {
    //autostart FPU when needed
    if(SCB->CFSR & SCB_CFSR_NOCP_Msk) {
        SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));    //start FPU
        return;
    }

    while(1);
}

__STACKLESS void MemManage_Handler(void) {
    while(1);   //futureAL[7]
}

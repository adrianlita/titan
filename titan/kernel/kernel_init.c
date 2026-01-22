#include <kernel.h>
#include <kernel_utils.h>
#include <assert.h>
#include <periph/cpu.h>

TITAN_KDEBUG_FILE_MARK;

void kernel_init(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF {
    STATIC_ASSERT(sizeof(void*) != 4);               //we must run on a 32bit machine
    STATIC_ASSERT(sizeof(task_state_t) != 4);      //task_state must have 4 bytes

    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    _kernel_tick = 0;
    kernel_task_init();
    kernel_primitive_init();
    kernel_scheduler_init();
}

void kernel_init_core(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    NVIC_SetPriorityGrouping(0);                //disable sub-priorities. every bit is used for pre-emption

    //setup PendSV and SVC
    NVIC_SetPriority(PendSV_IRQn, 0xFF);        //lowest priority to PendSV. this must remain as is for PendSV to be triggered by any IRQ when unlocking primitives
    NVIC_SetPriority(SVCall_IRQn, 0xFF);        //lowest priority to SVC_Handler. this must remain as is for SVC_Handler to be triggered by tasks only
    NVIC_EnableIRQ(SVCall_IRQn);

    //setup SysTick
    SysTick_Config(cpu_clock_speed() / 1000);

    NVIC_SetPriority(SysTick_IRQn, 0x00);       //highest priority to SysTick
    NVIC_EnableIRQ(SysTick_IRQn);
}

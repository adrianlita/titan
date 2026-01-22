#include <titan_load.h>
#include <titan.h>
#include <titan_defs.h>
#include <periph/cpu.h>
#include <kernel.h>
#include <task.h>

const char TITAN_VERSION[] = "0.1.1";
const char TITAN_BUILD_DATE[] = __DATE__;
const char TITAN_BUILD_TIME[] = __TIME__;

static titan_boot_reason_t titan_boot_reason;

/* idle task defs */
#ifndef TITAN_IDLE_TASK_STACK_SIZE
#define TITAN_IDLE_TASK_STACK_SIZE TASK_STACK_SIZE_MIN
static void kernel_idle_routine(void) {
    while(1) {
        __WFI();    //checkAL
    }
}
#else
extern void kernel_idle_routine(void);
#endif

static task_stack_t kernel_idle_stack[TITAN_IDLE_TASK_STACK_SIZE] in_task_stack_memory;
static task_t kernel_idle_task;

/* main() task defs */
#ifndef TITAN_MAIN_TASK_STACK_SIZE
#define TITAN_MAIN_TASK_STACK_SIZE TASK_STACK_SIZE_MIN
#endif

#ifndef TITAN_MAIN_TASK_PRIORITY
#define TITAN_MAIN_TASK_PRIORITY 1
#endif

#ifndef TITAN_MAIN_TASK_ROUTINE
#define TITAN_MAIN_TASK_ROUTINE   main
#else
#define __titan_entry_point         main
#endif

static void titan_early_run(void);
void TITAN_MAIN_TASK_ROUTINE(void);
void titan_setup(void);

static task_stack_t main_task_stack[TITAN_MAIN_TASK_STACK_SIZE] in_task_stack_memory;
task_t main_task;

#if (__FPU_USED == 1)
static void __titan_setup_fpu(void) {
    SCB->CPACR &= ~((3UL << 10*2)|(3UL << 11*2));   //set CP10,11 to access denied
    __asm volatile (
        "MRS    R0, CONTROL\n"
        "MOV    R0, #0\n"               //disable FP    checkAL asta e doar pt ca IAR face singur enable folosind un VMOV
        "MSR    CONTROL, R0\n"
    );

    //checkAL setup lazy stacking and everything
}
#endif

__NO_RETURN __STACKLESS void __titan_entry_point(void) {
    //initialize .bss with 0
    //initialize .data with .data_init
    
    //disable interrupts
    __disable_irq();
    #if (__FPU_USED == 1)
    __titan_setup_fpu();
    #endif

    //init bsp
    cpu_clock_init();
    cpu_boot_reason_get(&titan_boot_reason);

    /* init kernel */
    kernel_init();
    kernel_init_core();

    /* create idle task - must be first to get a task->id of 0 */
    task_init(&kernel_idle_task, kernel_idle_stack, TITAN_IDLE_TASK_STACK_SIZE, 0);
    task_attr_set_start_type(&kernel_idle_task, TASK_START_NOW, 0);
    task_create(&kernel_idle_task, kernel_idle_routine);

    /* create main() task */
    task_init(&main_task, main_task_stack, TITAN_MAIN_TASK_STACK_SIZE, TITAN_MAIN_TASK_PRIORITY);
    task_attr_set_start_type(&main_task, TASK_START_NOW, 0);
    task_create(&main_task, titan_early_run);

    //cancel context switch (as the two tasks above trigged it, but scheduler not yet started)
    SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;

    kernel_scheduler_start();
}

titan_boot_reason_t titan_get_boot_reason(void) {
    return titan_boot_reason;
}

static void titan_early_run(void) {
     __asm volatile (
        "LDR    R0, =titan_setup\n"
        "BLX    R0\n"
        "LDR    R0, ="STRINGIFY(TITAN_MAIN_TASK_ROUTINE)"\n"
        "BLX    R0\n"
    );
}

__WEAK void titan_setup(void) {
}

void titan_reboot(void) {
    NVIC_SystemReset();
}

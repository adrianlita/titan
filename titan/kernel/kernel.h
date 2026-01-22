#pragma once

#include <titan_defs.h>
#include <kernel_defines.h>
#include <kernel_atomic.h>

//exported variables
extern volatile uint32_t _kernel_tick;              //kernel internal tick
extern volatile task_t *kernel_current_task;    //holds current running task

/* kernel init APIs kernel_init.c */
void kernel_init(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF;                               //kernel init: initializes tructures and values
void kernel_init_core(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF;                          //kernel init: initializes basic clock, SysTick and PendSV

/* scheduler control APIs kernel_scheduler.c */
void kernel_scheduler_init(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF;                     //called by kernel_init()
__NO_RETURN __STACKLESS void kernel_scheduler_start(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_EXIT;           //called by titan_start(). assumes critical section is enabled

void kernel_scheduler_trigger(void) K_ISR_SAFE K_CSECT_NOMODIF;                                 //trigger context switch
void kernel_scheduler_yield(void) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;                      //make current task yield

void kernel_scheduler_task_start(task_t *task, task_state_t new_state) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;         //adds task to the respective queue (of new_state)
void kernel_scheduler_task_kill(task_t *task, void *retval) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;                      //kills task

void kernel_scheduler_task_change_priority(task_t *task, uint8_t priority) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;           //actively change priority of task

void kernel_scheduler_task_sleep_start(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF;                       //moves task to the voluntary sleep queue
void kernel_scheduler_task_sleep_finished(task_t *task) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;          //removes task from sleeping list and puts it into ready list. it also checks the waiting lists if task was waiting on something else besides sleep

void kernel_scheduler_task_wait_start(task_state_t new_state) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF;    //marks task as waiting (moves task to a respective waiting list) and yields CPU
void kernel_scheduler_task_wait_finished(void) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;                       //removes task from waiting list and puts it into ready list, but does not yield
void kernel_scheduler_task_notify(task_t *task, uint32_t mask) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;   //if notification is received by task, puts it into the ready list. does not yield

void kernel_scheduler_io_wait_start(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF;                        //marks task as waiting (for IO) and yields CPU
void kernel_scheduler_io_wait_finished(task_t *task) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;           //puts it into ready list, but does not yield

/* task APIs kernel_objects.c */
void kernel_task_init(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF;                            //called by kernel_init()
void kernel_task_create(task_t *task) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;                //allocate task id and maintain kernel task ids
void kernel_task_destroy(task_t *task) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;               //destroy task (when stopped or finished) and restore task id to the kernel for further usage

/* primitive APIs kernel_objects.c */
void kernel_primitive_init(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF;                         //called by kernel_init()
void kernel_primitive_create(primitive_t *primitive) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;       //allocate primitive id
void kernel_primitive_destroy(primitive_t *primitive) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF;      //destroy primitive

/* interrupts kernel_isr.c */
void SysTick_Handler(void);         //SysTick interrupt
void SVC_Handler(void);             //SVC Handler
void PendSV_Handler(void);          //PendSV interrupt
void HardFault_Handler(void);       //HardFault handler
void MemManage_Handler(void);       //MemManage_Handler handler

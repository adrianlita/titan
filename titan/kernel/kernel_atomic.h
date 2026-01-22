#pragma once

#include <stdint.h>
#include <assert.h>

/* function information defines */
#define K_ISR_FORBID            //function must never be called from an ISR
#define K_ISR_SAFE              //function safe to be called from an ISR

#define K_CSECT_MUST            //function that must be called from within a critical section. can be checked in the begining of function with K_SECT_MUST_CHECK

#define K_CSECT_ENTRY           //function when called will automatically enter a critical section
#define K_CSECT_EXIT            //function when called will automatically exit the critical section
#define K_CSECT_NOMODIF         //function does not modify the critical section status

/* debug macros */
#if (TITAN_KASSERT_ENABLE == 0)
    #define K_ISR_FORBID_CHECK
    #define K_CSECT_MUST_CHECK
#else
    #define K_ISR_FORBID_CHECK      kassert((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) == 0)
    #define K_CSECT_MUST_CHECK      kassert((__get_PRIMASK() != 0) || ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0))      //check if interrupts disabled, or inside an interrupt
#endif


/* kernel re-entrant critical sections. everything here is K_SECT_SAFE */
/* __disable/__enable_irq by themselves are faster but don't re-assure re-entrancy */
/* note that K_CSECT_BEGIN() and END() begins and ends a scope, so any variable inside will be discarded */
/* !!! ALSO note that it is not permitted to BEGIN() in a nested scope and END() in the parent scope !!! */
/* !!! if no new scope is wanted, then manually write them */

typedef uint32_t kernel_atomic_t;
#define KERNEL_CSECT_BEGIN()            \
{                                       \
    volatile kernel_atomic_t __atomic;  \
    kernel_begin_critical(&__atomic);

#define KERNEL_CSECT_END()              \
	kernel_end_critical(&__atomic);     \
}

void kernel_begin_critical(kernel_atomic_t volatile *atomic);   //mark the begining of a critical section by saving interrupts state and then disabling interrupts
void kernel_end_critical(kernel_atomic_t volatile *atomic);     //mark the leave from a critical section by restoring interrupts state

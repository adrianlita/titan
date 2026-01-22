#pragma once


/*  TITAN_DIGITAL_POWER_VOLTAGE - main power voltage, in mV
    - if undefined, will be assumed 3300 (3.3V)
    - checkAL unused yet, should be used for internal_flash
*/
#define TITAN_DIGITAL_POWER_VOLTAGE     3300

/*  TITAN_ANALOG_POWER_VOLTAGE - main power voltage, in mV
    - if undefined, will be assumed 3300 (3.3V)
    - checkAL unused yet
*/
#define TITAN_ANALOG_POWER_VOLTAGE      3300



/*
    TITAN_PERIPH_CRC32_USE_HARDWARE - mark whether to use hardware or software CRC32 function
    - prototype is uint32_t crc32(uint8_t buffer, uint32_t length)
    - if no hardware use can be done, unset this
    - note that by using hardware CRC32, only one task can use it at the same time (semaphore guarded)

    TITAN_PERIPH_CRC32_AUTO_HOUSEKEEPING - taken into account only if USE_HARDWARE is used
    - automatically init() and deinit() the CRC peripheral when crc32() is used
    - !!!! does NOT guard crc32_hard() at all
    - useful only if doing long buffers not very often (if often, don't disable the peripheral)
*/
#define TITAN_PERIPH_CRC32_USE_HARDWARE
#define TITAN_PERIPH_CRC32_AUTO_HOUSEKEEPING


/*  TITAN_RAND_FUNCTION - defines what is called when rand() is used
    - prototype is uint32_t rand(void);
    - if undefined, it automatically chooses rand1
    - be careful that rand1 needs to be initialized by srand1 (srand)
    - if other function is defined, its seed should be already setup and ready before using rand()
    
    - !!! Note that the usage of the TRNG is desired, but it is slow, and if a lot of randomness is needed, that might be a problem
    - also, whenever a pseudorandom function is used (such as rand1) it is best practice to refresh periodically the seed
    to something else, either from a TRNG or from another data crunching unit, especially if the PRNG algorithm is known
*/
#undef TITAN_RAND_FUNCTION

/*
    TITAN_ASSERT_LEVEL - assertion level to implement. default value if not implmeneted is 0xFF
    - 0 = no assertion
    - 1 = simple assert, no information provided
    - 2 = assert with line number
    - 3 = assert with full filename, and line number
    - >3 = same as 3 (so the list may continue)

    TITAN_SASSERT_ENABLE - system functions assertions enable. system functions are usually called by user
                         - does not check on deep kernel assertions (see TITAN_KASSERT_ENABLE)
    - 0 = no assertion
    - 1 = same level as TITAN_ASSERT_LEVEL

    TITAN_KASSERT_ENABLE - deep kernel assertion enable (kernel assertions are NOT invoked by kernel objects (see TITAN_SASSERT_ENABLE))
                         - also includes K_ISR_CHECK and K_CSECT_* checks
    - 0 = no assertion
    - 1 = same level as TITAN_ASSERT_LEVEL

    TITAN_PASSERT_ENABLE - peripheral assertion enable
    - 0 = no assertion
    - 1 = same level as TITAN_ASSERT_LEVEL
*/
#define TITAN_ASSERT_LEVEL                  3
#define TITAN_SASSERT_ENABLE                1
#define TITAN_KASSERT_ENABLE                1
#define TITAN_PASSERT_ENABLE                1

/*
    TITAN_KERNEL_TICKLESS_OPERATION - defines whether the kernel runs in a tickless mode or not
    - note that tickless operation automatically means preemptive, COOPERATIVE scheduler. if two tasks with max_current_priority runs, they must block/yield to let others do their job
    - futureAL[3]
*/
#define TITAN_KERNEL_TICKLESS_OPERATION

/*
    TITAN_IDLE_TASK_STACK_SIZE - defines stack size of the idle task
    - must be minimum TASK_STACK_SIZE_MIN. 
    - must be a multiple of 8
    - if undefined here, it is automatically defined to TASK_STACK_SIZE_MIN
    - if this is defined it is automatically implied that the user implements kernel_idle_routine()
*/
#undef TITAN_IDLE_TASK_STACK_SIZE

/*
    TITAN_MAIN_TASK_ROUTINE   - defines the routine (function) name of the main() task
    - if undefined, it is assumed to be main(), but some compilers don't support this
    - if defined, then __titan_entry_point is defined as main()

    TITAN_MAIN_TASK_STACK_SIZE - defines stack size of the main() task
    - must be minimum TASK_STACK_SIZE_MIN
    - must be a multiple of 8
    - if undefined here, it is automatically defined to TASK_STACK_SIZE_MIN
    
    TITAN_MAIN_TASK_PRIORITY - defines the initial priority of the main() task
    - must be minimum 1
    - if undefined here, it is automatically defined to 1
*/
#define TITAN_MAIN_TASK_ROUTINE       app_main
#define TITAN_MAIN_TASK_STACK_SIZE    1024
#undef TITAN_MAIN_TASK_PRIORITY

/*
    When using FPU, please regard that:
    - configuration will be disregarded if:
        1. no FPU is present in the micro
        2. FPU must be also be enabled in compiler's settings in order to generate FPU instructions
        
    - FPU does floating point operations faster, but consumes power when on
    - TITAN does not use FPU in *any* of the system components
    - if any driver or 3rd party library uses FPU, it *should* automatically check for flags defined below and inform the programmer if anything not compliant

    - TITAN automatically starts FPU whenever needed
    - FPU can be turned off manually when not needed for a long period of time
    

    TITAN_FPU_USED_IN_ONE_TASK_ONLY - define this if only one of your tasks (or ISR) is using FPU
    - FPU context will *NOT* be saved automatically, which does not impact speed at all
    - if defined and more than one task or an ISR is using it, the behaviour will be undefined
*/
#define TITAN_FPU_USED_IN_ONE_TASK_ONLY



/*
    TITAN EXPLORER utility

    TITAN_EXPLORER_OUTPUT_PACKET_SIZE
    - used for outputting data from titan to external
    - maximum message size, including special characters that the explorer_log() function can work with
    - min value is 16
    - should be a multiple of 8

    TITAN_EXPLORER_LOG_PAYLOAD_ONLY
    - this allows log() function to send data without any headers or checksum
    - when used this way, PLOT will not be functional
    - must NOT be defined if using Titan Explorer
*/
#define TITAN_EXPLORER_OUTPUT_PACKET_SIZE       512
#define TITAN_EXPLORER_LOG_PAYLOAD_ONLY





//finally define MCU and include its header alongside CMSIS
#define STM32L452xx
#include <stm32l4xx.h>

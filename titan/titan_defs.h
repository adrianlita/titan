#pragma once

#include <stdint.h>

#define TASK_KEY_VALUE                0xDEDEDEDEU         //value for the task_initialized key
#define TASK_STACK_INIT_VALUE         0xF8EEF8EEU         //free
#define TASK_STACK_DEFAULT_SIZE       ((uint32_t)(48))    //in stack words


#if (__FPU_USED == 1)
    #if defined(TITAN_FPU_USED_IN_ONE_TASK_ONLY)
        #define KERNEL_FPU_ALWAYS_SAVING_CONTEXT        0
    #else
        #define KERNEL_FPU_ALWAYS_SAVING_CONTEXT        1
    #endif
#else
    #define KERNEL_FPU_ALWAYS_SAVING_CONTEXT            0
#endif

#if (KERNEL_FPU_ALWAYS_SAVING_CONTEXT == 1)
    #define TASK_STACK_SIZE_MIN       (TASK_STACK_DEFAULT_SIZE + 40)
#else
    #define TASK_STACK_SIZE_MIN       TASK_STACK_DEFAULT_SIZE
#endif






/* stringify C macro */
#define VAL(x) #x
#define STRINGIFY(x) VAL(x)

/* stackless */
#if defined ( __GNUC__ )
    #define __STACKLESS __attribute__ ((naked, optimize("-fno-stack-protector")))
#elif defined ( __ICCARM__ )
    #define __STACKLESS __stackless
#endif

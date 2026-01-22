#pragma once

#include <stdint.h>

/* also defined like this so they can work with the assembly part of the code */
#define _TS_READY_      0x00000001
#define _TS_RUNNING_    0x00000002

/* task state */
typedef enum {
    TS_UNUSED       = 0,                    //task not used in scheduling
    TS_READY        = _TS_READY_,           //task ready to run (will be scheduled at some moment)
    TS_RUNNING      = _TS_RUNNING_,         //task is currently running

    TS_WAIT_SLEEP   = 0x00000004,           //task is waiting for a period of time (called sleep), but can be aborted

    //primitives
    TS_WAIT_JOIN    = 0x00000008,           //task is waiting for another task to join
    TS_WAIT_SEMA    = 0x00000010,           //task is waiting for a semaphore
    TS_WAIT_MUTEX   = 0x00000020,           //task is waiting for a mutex
    TS_WAIT_MQ_IN   = 0x00000040,           //task is waiting for a message queue send
    TS_WAIT_MQ_OUT  = 0x00000080,           //task is waiting for a message queue receive

    //notifications
    TS_WAIT_NOTIFY  = 0x00000100,           //task is waiting for a notification (note that notification is NOT a primitive)

    //io
    TS_WAIT_IO      = 0x01000000,           //task is waiting for IO to finish, with no actual 

    //not-running
    TS_STOPPED      = 0x10000000,           //task stopped (not started)
    TS_EXITED       = 0x20000000,           //task exited
    TS_KILLED       = 0x40000000,           //task killed
    
    //not actual real states, but helps on coding
    TS_WAITING_PRIM = TS_WAIT_SEMA | TS_WAIT_MUTEX | TS_WAIT_MQ_IN | TS_WAIT_MQ_OUT,
    TS_WAITING      = TS_WAIT_SLEEP | TS_WAIT_JOIN | TS_WAITING_PRIM | TS_WAIT_NOTIFY,
    TS_STARTED      = TS_READY | TS_RUNNING | TS_WAITING,
    TS_FINISHED     = TS_EXITED | TS_KILLED,    //zombie tasks
    TS_NOT_STARTED  = TS_STOPPED | TS_FINISHED,
    TS_DESTROYABLE  = TS_STOPPED | TS_FINISHED,

    TS_FORCE_UINT32 = 0x7FFFFFFF            //just to force a uint32_t enum. checked in kernel_init() afterwards
} task_state_t;

/* task start types */
typedef enum {
    TASK_START_NOW = 0,           //(default value) task will be scheduled right away
    TASK_START_MANUAL = 1,        //task won't be started until task_start(task) is called
    TASK_START_DELAYED = 2,       //task will start after a delay. task_wake will also work
} task_start_type_t;

/* task id */
typedef uint32_t task_id_t;

/* task routine */
typedef void*(*task_routine_t)(void*);

#define TASK_WAIT_REASON_INIT         0x01     //primitive init
#define TASK_WAIT_REASON_UNLOCKED     0x02     //awoken because primitive was unlocked
#define TASK_WAIT_REASON_SLEEP        0x04     //awoken because timeout has expired

/* ptask stack unit */
typedef uint32_t task_stack_t;
#define TASK_STACK_SIZE(x)    (sizeof(x)/sizeof(x[0]))
#define in_task_stack_memory  __ALIGNED(8)

/* task control block - kernel side */
typedef struct __task_s {
    void *stack_pointer;            //stack pointer, will point to the max address. MUST BE TOP OF TCB
    task_state_t state;             //task state. MUST BE SECOND TOP OF TCB
    task_id_t id;                   //task id
    struct __task_s *prev;          //prev task on its state-list
    struct __task_s *next;          //next task on its state-list

    struct __task_s *sprev;         //prev task on sleeping list
    struct __task_s *snext;         //next task on sleeping list

    uint8_t prio;                   //task priority. lower means lower
    uint32_t timeout;               //task is sleeping until timeout
    struct __task_s *join_head;     //tasks waiting to join this task

    void *wait_object;              //object that task is currently waiting uppon (except sleeps)
                                    //so far, can be a primitive or a task (for joining)
    uint32_t wait_reason;           //wait reason on return

    void *stack_min;                //end of stack address (used for stack-restart)
    uint32_t stack_size;            //total stack size
    task_routine_t start_routine; //beging of task. used for restarting
    void *retval;                   //return value (if task is one that returns)

    uint32_t notif_active;          //mask for active notifications
    uint32_t notif_pending;         //pending notifications

    uint32_t key;                   //key for making sure attr are intialized
    task_start_type_t start_type;
} task_t;

typedef uint32_t primitive_id_t;

typedef struct __primitive {
    primitive_id_t id;                  //primitive id
    task_t *waiting_queue;              //task waiting queue
} primitive_t;

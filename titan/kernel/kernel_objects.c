#include <kernel.h>
#include <kernel_utils.h>
#include <assert.h>

TITAN_KDEBUG_FILE_MARK;   //useful in debug, otherwise not taken into account

/****************************/
/* Objects: tasks         */
/****************************/

extern volatile task_t *kernel_current_task;
extern volatile task_t *kernel_ready_queue;
extern volatile task_t *kernel_sleep_queue;

static task_id_t kernel_task_id_min;    //minimum tid on current pass that is unused

void kernel_task_init(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    kernel_task_id_min = 0;
}

void kernel_task_create(task_t *task) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    task->id = kernel_task_id_min;   //update tid
    kernel_task_id_min++;

    //futureAL[4]
}

void kernel_task_destroy(task_t *task) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    kassert(task != 0);
    kassert(task->id != 0);
    kassert((task->state & TS_DESTROYABLE) != 0);

    task->state = TS_UNUSED;
    task->key = 0;

    //futureAL[5]
}


/****************************/
/* Objects: primitives      */
/****************************/
 
static primitive_id_t kernel_primitive_id_min;          //minimum primitive id on current pass that is unused

void kernel_primitive_init(void) K_ISR_FORBID K_CSECT_MUST K_CSECT_NOMODIF {
    K_ISR_FORBID_CHECK;
    K_CSECT_MUST_CHECK;

    kernel_primitive_id_min = 0;
}

void kernel_primitive_create(primitive_t *primitive) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;

    kassert(primitive != 0);

    primitive->id = kernel_primitive_id_min;
    kernel_primitive_id_min++;
    
    primitive->waiting_queue = 0;
}

void kernel_primitive_destroy(primitive_t *primitive) K_ISR_SAFE K_CSECT_MUST K_CSECT_NOMODIF {
    K_CSECT_MUST_CHECK;
    
    //checkAL
}

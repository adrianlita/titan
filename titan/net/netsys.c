#include "net_task.h"
#include <task.h>
#include <titan_defs.h>

#define TITAN_NET_SUBSYSTEM_TASK_STACK_SIZE TASK_STACK_SIZE_MIN
#define TITAN_NET_SUBSYSTEM_TASK_PRIORITY   1

static task_stack_t net_task_stack[TITAN_NET_SUBSYSTEM_TASK_STACK_SIZE] in_task_stack_memory;
static task_t net_task;

static void net_task_routine(void);

void net_task_init(void) {
    task_init(&net_task, net_task_stack, TITAN_NET_SUBSYSTEM_TASK_STACK_SIZE, TITAN_NET_SUBSYSTEM_TASK_PRIORITY);
    task_attr_set_start_type(&net_task, TASK_START_NOW, 0);
    task_create(&net_task, net_task_routine);
}

static void net_task_routine(void) {

}

#pragma once

#include <task.h>

typedef enum {
    TITAN_BOOT_REASON_UNAVAILABLE = 0,  //boot reason is unavailable
    TITAN_BOOT_REASON_NORMAL = 1,       //normal startup
    TITAN_BOOT_REASON_USER = 2,         //user called titan_reboot
    TITAN_BOOT_REASON_HARDWARE = 3,     //hardware reason (button, BOR, POR, etc)
    TITAN_BOOT_REASON_WATCHDOG = 4,     //hardware watchdog not kicked on time
} titan_boot_reason_t;

extern const char TITAN_VERSION[];
extern const char TITAN_BUILD_DATE[];
extern const char TITAN_BUILD_TIME[];

extern task_t main_task;

titan_boot_reason_t titan_get_boot_reason(void);
void titan_reboot(void);

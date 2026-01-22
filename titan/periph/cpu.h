#pragma once

#include <stdint.h>
#include <titan.h>

void cpu_clock_init(void);
uint32_t cpu_clock_speed(void);

void cpu_boot_reason_get(titan_boot_reason_t *store);

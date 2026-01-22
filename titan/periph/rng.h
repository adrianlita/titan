#pragma once

#include <stdint.h>

void rng_init(void);
void rng_deinit(void);

void rng_random(uint32_t *pool, uint32_t pool_length);

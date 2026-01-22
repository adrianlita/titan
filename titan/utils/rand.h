#pragma once

#include <stdint.h>

//main functions to be used
void srand(const uint32_t *seed);
uint32_t rand(void);

//different implementations
void srand1(const uint32_t *seed);
uint32_t rand1(void);

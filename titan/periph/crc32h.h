#pragma once

#include <stdint.h>

void crc32_hard_init(void);
void crc32_hard_deinit(void);

uint32_t crc32_hard(const uint8_t *buffer, const uint32_t length);

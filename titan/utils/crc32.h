#pragma once

#include <stdint.h>

uint32_t crc32(const uint8_t *buffer, const uint32_t length);       //soft or hard implementation chosen automatically
uint32_t crc32_soft(const uint8_t *buffer, const uint32_t length);  //calculate using soft method

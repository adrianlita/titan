#pragma once
#include <stdint.h>

uint32_t base64_decode_get_len(const char *bufcoded);
uint32_t base64_decode(char *bufplain, const char *bufcoded);

uint32_t base64_encode_get_len(uint32_t plain_buf_len);
uint32_t base64_encode(char *encoded, const char *bufplain, int32_t buflen);

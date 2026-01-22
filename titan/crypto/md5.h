#pragma once

#include <stdint.h>

/*
    based on code from Brad Conte (brad AT bradconte.com)
    modified to fit
*/

#define MD5_BLOCK_SIZE      16               // MD5 outputs a 16 byte digest

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[4];
} md5_ctx_t;

/*********************** FUNCTION DECLARATIONS **********************/
void md5_init_ctx(md5_ctx_t *ctx);
void md5_update(md5_ctx_t *ctx, const uint8_t *data, uint32_t length);
void md5_final(md5_ctx_t *ctx, uint8_t *hash);

/*
    usage:

    #include <crypto/md5.h>

    uint8_t hash[MD5_BLOCK_SIZE];
    md5_ctx_t ctx;

    md5_init_ctx(&ctx);
    md5_update(&ctx, "abcd", 4);
    md5_update(&ctx, "efghi", 5);
    md5_final(&ctx, hash);

    -- for another hash, you must init_ctx(&ctx);
*/

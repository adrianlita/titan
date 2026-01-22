#pragma once

#include <stdint.h>

/*
    based on code from Brad Conte (brad AT bradconte.com)
    modified to fit
*/

#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} sha256_ctx_t;

void sha256_init_ctx(sha256_ctx_t *ctx);
void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, uint32_t length);
void sha256_final(sha256_ctx_t *ctx, uint8_t *hash);

/*
    usage:

    #include <crypto/sha256.h>

    uint8_t hash[SHA256_BLOCK_SIZE];
    sha256_ctx_t ctx;

    sha256_init_ctx(&ctx);
    sha256_update(&ctx, "abcd", 4);
    sha256_update(&ctx, "efghi", 5);
    sha256_final(&ctx, hash);

    -- for another hash, you must init_ctx(&ctx);
*/

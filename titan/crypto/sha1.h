#pragma once

#include <stdint.h>

/*
    based on code from Brad Conte (brad AT bradconte.com)
    modified to fit
*/

#define SHA1_BLOCK_SIZE 20            // SHA1 outputs a 20 byte digest

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[5];
    uint32_t k[4];
} sha1_ctx_t;

void sha1_init_ctx(sha1_ctx_t *ctx);
void sha1_update(sha1_ctx_t *ctx, const uint8_t *data, uint32_t length);
void sha1_final(sha1_ctx_t *ctx, uint8_t *hash);

/*
    usage:

    #include <crypto/sha1.h>

    uint8_t hash[SHA1_BLOCK_SIZE];
    sha1_ctx_t ctx;

    sha1_init_ctx(&ctx);
    sha1_update(&ctx, "abcd", 4);
    sha1_update(&ctx, "efghi", 5);
    sha1_final(&ctx, hash);

    -- for another hash, you must init_ctx(&ctx);
*/

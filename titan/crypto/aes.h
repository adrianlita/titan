#pragma once

#include <stdint.h>

/* 
    based on the code found here https://github.com/kokke/tiny-AES-c 
    but heavily modified
*/

#define AES_BLOCKLEN            16      // Block length in bytes - AES is 128b block only

typedef enum {
    AES128 = 16,
    AES192 = 24,
    AES256 = 32,
} aes_key_size_t;

typedef struct __aes_ctx {
    aes_key_size_t key_size;

    uint8_t round_key[240];
    uint8_t iv[AES_BLOCKLEN];
} aes_ctx_t;

void aes_init_ctx(aes_ctx_t *ctx, aes_key_size_t key_size, const uint8_t *key, const uint8_t *iv);
void aes_reset_iv(aes_ctx_t *ctx, const uint8_t *iv);

uint32_t aes_pad_buffer_pkcs7(uint8_t *buf, uint32_t length);           //returns new length

void aes_cbc_encrypt_buffer(aes_ctx_t *ctx, uint8_t *buf, uint32_t length);
void aes_cbc_decrypt_buffer(aes_ctx_t *ctx, uint8_t *buf, uint32_t length);

/*
    usage:

    #include <crypto/aes.h>

    aes_ctx_t ctx;

    aes_init_ctx(&ctx, AES256, key, iv);
    -   iv can be NULL, and it won't get initialized
    -   this is done to allow faster calls and initialize directly through ctx.iv  (ex: rand_pool(ctx.iv, AES_BLOCKLEN));

    aes_reset_iv
    -   is the same as working directly with ctx.iv, no difference

    aes_pad_buffer_pkcs7
    -   pads buf up to a len divisible by AES_BLOCKSIZE
    -   returns new length (which will be divisible by AES_BLOCKSIZE)
    -   buf **MUST** have enough space to accomodate the padding

    aes_cbc_encrypt_buffer
    aes_cbc_decrypt_buffer
    -   context MUST be initialized before using them, and after **EACH** usage
    -   do their jobs in place
    -   length must be divisible by AES_BLOCKSIZE otherwise will assert




    EXAMPLE:
    
    uint8_t key[32];
    uint8_t iv[16];
    uint8_t buf[16];
    for(uint8_t i = 0; i < AES_BLOCKLEN; i++) {
        aes_ctx.iv[i] = 0;
        key[i] = 0;
        key[16 + i] = 0;
        iv[i] = 0;
        buf[i] = 0;
    }
    key[0] = 0x80;

    for(int i = 0; i < 16; i++) {
        explorer_log("%02X", buf[i]);
    }
    explorer_log("\r\n");

    aes_init_ctx(&aes_ctx, AES128, key, iv);
    aes_cbc_encrypt_buffer(&aes_ctx, buf, 16);

    for(int i = 0; i < 16; i++) {
        explorer_log("%02X", buf[i]);
    }
    explorer_log("\r\n");

    aes_init_ctx(&aes_ctx, AES128, key, iv);
    aes_cbc_decrypt_buffer(&aes_ctx, buf, 16);

    for(int i = 0; i < 16; i++) {
        explorer_log("%02X", buf[i]);
    }
    explorer_log("\r\n\r\n");
*/

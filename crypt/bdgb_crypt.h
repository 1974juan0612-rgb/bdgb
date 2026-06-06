#ifndef BDGB_CRYPT_H
#define BDGB_CRYPT_H

#include <stdint.h>
#include <stddef.h>

#define BDGB_CRYPT_MAGIC    "BDGB"
#define BDGB_CRYPT_VERSION  1
#define BDGB_CRYPT_IV_SIZE  8
#define BDGB_CRYPT_KEY_MAX  64
#define BDGB_CRYPT_ROUNDS   4

typedef struct {
    uint8_t seed;
    uint32_t wseed[4];
    uint8_t iv[BDGB_CRYPT_IV_SIZE];
    uint64_t counter;
} BDGBCryptCtx;

int  bdgb_crypt_init(BDGBCryptCtx *ctx, const char *password,
                     const uint8_t *iv);

void bdgb_crypt_iv_generate(uint8_t *iv);

void bdgb_crypt_keystream(BDGBCryptCtx *ctx, uint8_t *out, size_t len);

void bdgb_crypt_buffer(BDGBCryptCtx *ctx, uint8_t *data, size_t len);

int  bdgb_encrypt_file(const char *in_path, const char *out_path,
                       const char *password);

int  bdgb_decrypt_file(const char *in_path, const char *out_path,
                       const char *password);

#endif

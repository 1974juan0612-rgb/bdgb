#include "bdgb_crypt.h"
#include "bdgb.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/*
 * BDGB-Cipher: stream cipher based on the BDGB mathematical model.
 *
 * Design:
 * - 32-bit internal state (4 bytes, treated as a 32-bit word)
 * - Key derivation uses BDGB geometric properties as a non-linear S-box
 *   but avoids repeated dynamic_rule() which converges in 8-bit space
 * - State evolution uses rotate, XOR, and BDGB property mixing
 * - 16-byte block processing (one grid row per block)
 */

#define ROTL8(x, n) (((x) << (n)) | ((x) >> (8 - (n))))
#define ROTR8(x, n) (((x) >> (n)) | ((x) << (8 - (n))))

static uint8_t bdgb_sbox(uint8_t x) {
    bdgb_props_t p = bdgb_compute_props(x);
    uint8_t r = x ^ (p.densidad << 5) ^ (p.radio << 3)
              ^ (p.simetria << 2) ^ (p.tipo_geom << 1);
    r ^= (uint8_t)(bdgb_steps_to_attractor(x) * 0x3B);
    return r ^ (uint8_t)(bdgb_count_ones(x) * 0xD7);
}

static uint32_t mix32(uint32_t w) {
    uint8_t b0 = (uint8_t)(w & 0xFF);
    uint8_t b1 = (uint8_t)((w >> 8) & 0xFF);
    uint8_t b2 = (uint8_t)((w >> 16) & 0xFF);
    uint8_t b3 = (uint8_t)((w >> 24) & 0xFF);
    b0 = bdgb_sbox(b0); b1 = bdgb_sbox(b1);
    b2 = bdgb_sbox(b2); b3 = bdgb_sbox(b3);
    b0 = ROTL8(b0, 3); b1 = ROTL8(b1, 5);
    b2 = ROTL8(b2, 7); b3 = ROTL8(b3, 1);
    uint32_t out = ((uint32_t)b0) | ((uint32_t)b1 << 8)
                 | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
    out ^= ROTL8((uint8_t)(out >> 16), 4) * 0x01010101u;
    out += (uint32_t)bdgb_count_ones(b0 ^ b1 ^ b2 ^ b3) * 0x9E3779B9u;
    return out;
}

static void derive_seed_32(const char *password, const uint8_t *iv,
                           uint32_t seed[4]) {
    seed[0] = 0x6C078965u;
    seed[1] = 0x5EED0D15u;
    seed[2] = 0x3C6EF37Fu;
    seed[3] = 0xA54FF53Au;

    size_t plen = password ? strlen(password) : 0;
    size_t total = plen + (iv ? BDGB_CRYPT_IV_SIZE : 0);
    if (total == 0) return;

    uint8_t buf[256];
    size_t pos = 0;
    for (size_t i = 0; i < plen; i++) buf[pos++] = (uint8_t)password[i];
    if (iv) for (int i = 0; i < BDGB_CRYPT_IV_SIZE; i++) buf[pos++] = iv[i];

    for (size_t feed = 0; feed < pos; feed++) {
        int wi = feed % 4;
        seed[wi] ^= (uint32_t)buf[feed] * 0x01010101u;
        seed[wi] = mix32(seed[wi]);
        seed[(wi + 1) % 4] += seed[wi] ^ ROTL8(buf[feed], 3);
        seed[(wi + 2) % 4] ^= seed[(wi + 1) % 4]
                            + (uint32_t)bdgb_count_ones(buf[feed]) * 0x3B9ACA07u;
    }

    for (int r = 0; r < 8; r++) {
        for (int i = 0; i < 4; i++) {
            seed[i] = mix32(seed[i]);
            seed[(i + 1) % 4] ^= seed[i];
            seed[(i + 2) % 4] += seed[i] ^ ROTL8((uint8_t)(seed[i] >> 16), r * 3 + 1);
        }
    }
}

void bdgb_crypt_iv_generate(uint8_t *iv) {
    for (int i = 0; i < BDGB_CRYPT_IV_SIZE; i++) {
        iv[i] = (uint8_t)(rand() & 0xFF);
        iv[i] ^= (uint8_t)(rand() >> 8);
        iv[i] = bdgb_sbox(iv[i] ^ (uint8_t)(i * 0x7F));
    }
}

int bdgb_crypt_init(BDGBCryptCtx *ctx, const char *password,
                    const uint8_t *iv) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(BDGBCryptCtx));
    if (iv) memcpy(ctx->iv, iv, BDGB_CRYPT_IV_SIZE);
    derive_seed_32(password, iv, ctx->wseed);
    ctx->counter = 0;
    ctx->seed = (uint8_t)(ctx->wseed[0] ^ ctx->wseed[1]
                         ^ ctx->wseed[2] ^ ctx->wseed[3]);
    return 0;
}

void bdgb_crypt_keystream(BDGBCryptCtx *ctx, uint8_t *out, size_t len) {
    if (!ctx || !out || len == 0) return;

    for (size_t i = 0; i < len; i++) {
        uint8_t fb = (uint8_t)((ctx->counter & 0xFF)
                     ^ ((ctx->counter >> 8) & 0xFF));

        ctx->wseed[0] = mix32(ctx->wseed[0] ^ fb);
        ctx->wseed[1] = mix32(ctx->wseed[1] + ROTL8(fb, 3));
        ctx->wseed[2] = mix32(ctx->wseed[2] ^ (uint32_t)fb * 0x9E3779B9u);
        ctx->wseed[3] = mix32(ctx->wseed[3] + (uint32_t)(fb ^ 0xA5));

        uint8_t k = (uint8_t)(ctx->wseed[0] & 0xFF)
                  ^ (uint8_t)((ctx->wseed[1] >> 8) & 0xFF)
                  ^ (uint8_t)((ctx->wseed[2] >> 16) & 0xFF)
                  ^ (uint8_t)((ctx->wseed[3] >> 24) & 0xFF);

        k ^= (uint8_t)(ctx->counter & 0xFF);
        k ^= (uint8_t)((ctx->counter >> 4) & 0xF0);
        k = bdgb_sbox(k ^ (uint8_t)(ctx->counter >> 8));

        ctx->counter++;
        out[i] = k;
    }
}

void bdgb_crypt_buffer(BDGBCryptCtx *ctx, uint8_t *data, size_t len) {
    if (!ctx || !data || len == 0) return;
    uint8_t *ks = (uint8_t*)malloc(len);
    if (!ks) return;
    bdgb_crypt_keystream(ctx, ks, len);
    for (size_t i = 0; i < len; i++) data[i] ^= ks[i];
    free(ks);
}

int bdgb_encrypt_file(const char *in_path, const char *out_path,
                      const char *password) {
    if (!in_path || !out_path || !password) return -1;
    FILE *fin = fopen(in_path, "rb");
    if (!fin) return -1;
    FILE *fout = fopen(out_path, "wb");
    if (!fout) { fclose(fin); return -1; }

    fwrite(BDGB_CRYPT_MAGIC, 1, 4, fout);
    uint8_t version = BDGB_CRYPT_VERSION;
    fwrite(&version, 1, 1, fout);
    uint8_t iv[BDGB_CRYPT_IV_SIZE];
    bdgb_crypt_iv_generate(iv);
    fwrite(iv, 1, BDGB_CRYPT_IV_SIZE, fout);

    BDGBCryptCtx ctx;
    bdgb_crypt_init(&ctx, password, iv);
    uint8_t buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0) {
        bdgb_crypt_buffer(&ctx, buf, n);
        fwrite(buf, 1, n, fout);
    }
    fclose(fin); fclose(fout);
    return 0;
}

int bdgb_decrypt_file(const char *in_path, const char *out_path,
                      const char *password) {
    if (!in_path || !out_path || !password) return -1;
    FILE *fin = fopen(in_path, "rb");
    if (!fin) return -1;

    char magic[5] = {0};
    size_t n;
    if ((n = fread(magic, 1, 4, fin)) != 4
        || strncmp(magic, BDGB_CRYPT_MAGIC, 4) != 0) {
        fclose(fin); return -1;
    }
    uint8_t version;
    if (fread(&version, 1, 1, fin) != 1 || version != BDGB_CRYPT_VERSION) {
        fclose(fin); return -1;
    }
    uint8_t iv[BDGB_CRYPT_IV_SIZE];
    if (fread(iv, 1, BDGB_CRYPT_IV_SIZE, fin) != BDGB_CRYPT_IV_SIZE) {
        fclose(fin); return -1;
    }

    FILE *fout = fopen(out_path, "wb");
    if (!fout) { fclose(fin); return -1; }

    BDGBCryptCtx ctx;
    bdgb_crypt_init(&ctx, password, iv);
    uint8_t buf[4096];
    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0) {
        bdgb_crypt_buffer(&ctx, buf, n);
        if (fwrite(buf, 1, n, fout) != n) {
            fclose(fin); fclose(fout);
            remove(out_path);
            return -1;
        }
    }
    fclose(fin); fclose(fout);
    return 0;
}

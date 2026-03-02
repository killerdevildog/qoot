/*
 * qemt - SHA-256 hash implementation (FIPS 180-4)
 * No glibc dependency.
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifndef QEMT_SHA256_H
#define QEMT_SHA256_H

#include "types.h"
#include "string.h"

typedef struct {
    unsigned int state[8];
    unsigned long count;
    unsigned char buffer[64];
} sha256_ctx_t;

#define SHA256_CH(x,y,z)  (((x)&(y))^((~(x))&(z)))
#define SHA256_MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define SHA256_ROTR(x,n)  (((x)>>(n))|((x)<<(32-(n))))
#define SHA256_SIG0(x) (SHA256_ROTR(x,2)^SHA256_ROTR(x,13)^SHA256_ROTR(x,22))
#define SHA256_SIG1(x) (SHA256_ROTR(x,6)^SHA256_ROTR(x,11)^SHA256_ROTR(x,25))
#define SHA256_sig0(x) (SHA256_ROTR(x,7)^SHA256_ROTR(x,18)^((x)>>3))
#define SHA256_sig1(x) (SHA256_ROTR(x,17)^SHA256_ROTR(x,19)^((x)>>10))

static const unsigned int sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static inline void sha256_transform(sha256_ctx_t *ctx) {
    unsigned int w[64];
    unsigned int a,b,c,d,e,f,g,h;

    for (int i = 0; i < 16; i++)
        w[i] = ((unsigned int)ctx->buffer[i*4] << 24) |
               ((unsigned int)ctx->buffer[i*4+1] << 16) |
               ((unsigned int)ctx->buffer[i*4+2] << 8) |
               ((unsigned int)ctx->buffer[i*4+3]);

    for (int i = 16; i < 64; i++)
        w[i] = SHA256_sig1(w[i-2]) + w[i-7] + SHA256_sig0(w[i-15]) + w[i-16];

    a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
    e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];

    for (int i = 0; i < 64; i++) {
        unsigned int t1 = h + SHA256_SIG1(e) + SHA256_CH(e,f,g) + sha256_k[i] + w[i];
        unsigned int t2 = SHA256_SIG0(a) + SHA256_MAJ(a,b,c);
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }

    ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c; ctx->state[3]+=d;
    ctx->state[4]+=e; ctx->state[5]+=f; ctx->state[6]+=g; ctx->state[7]+=h;
}

static inline void sha256_init(sha256_ctx_t *ctx) {
    ctx->state[0]=0x6a09e667; ctx->state[1]=0xbb67ae85;
    ctx->state[2]=0x3c6ef372; ctx->state[3]=0xa54ff53a;
    ctx->state[4]=0x510e527f; ctx->state[5]=0x9b05688c;
    ctx->state[6]=0x1f83d9ab; ctx->state[7]=0x5be0cd19;
    ctx->count = 0;
    mem_set(ctx->buffer, 0, 64);
}

static inline void sha256_update(sha256_ctx_t *ctx, const unsigned char *data, unsigned long len) {
    for (unsigned long i = 0; i < len; i++) {
        ctx->buffer[ctx->count % 64] = data[i];
        ctx->count++;
        if (ctx->count % 64 == 0)
            sha256_transform(ctx);
    }
}

static inline void sha256_final(sha256_ctx_t *ctx, unsigned char digest[32]) {
    unsigned long bits = ctx->count * 8;
    unsigned long idx = ctx->count % 64;

    ctx->buffer[idx++] = 0x80;
    if (idx > 56) {
        while (idx < 64) ctx->buffer[idx++] = 0;
        sha256_transform(ctx);
        idx = 0;
    }
    while (idx < 56) ctx->buffer[idx++] = 0;

    /* Big-endian bit count */
    for (int i = 0; i < 8; i++)
        ctx->buffer[56 + i] = (unsigned char)(bits >> ((7 - i) * 8));

    sha256_transform(ctx);

    for (int i = 0; i < 8; i++) {
        digest[i*4]   = (unsigned char)(ctx->state[i] >> 24);
        digest[i*4+1] = (unsigned char)(ctx->state[i] >> 16);
        digest[i*4+2] = (unsigned char)(ctx->state[i] >> 8);
        digest[i*4+3] = (unsigned char)(ctx->state[i]);
    }
}

#endif /* QEMT_SHA256_H */

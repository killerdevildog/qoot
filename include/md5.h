/*
 * qemt - MD5 hash implementation (RFC 1321)
 * No glibc dependency.
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifndef QEMT_MD5_H
#define QEMT_MD5_H

#include "types.h"
#include "string.h"

typedef struct {
    unsigned int state[4];
    unsigned long count;
    unsigned char buffer[64];
} md5_ctx_t;

#define MD5_F(x,y,z) (((x)&(y))|((~(x))&(z)))
#define MD5_G(x,y,z) (((x)&(z))|((y)&(~(z))))
#define MD5_H(x,y,z) ((x)^(y)^(z))
#define MD5_I(x,y,z) ((y)^((x)|(~(z))))
#define MD5_ROT(x,n) (((x)<<(n))|((x)>>(32-(n))))

static const unsigned int md5_k[] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};
static const unsigned char md5_r[] = {
    7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
    5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
    6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
};

static inline void md5_transform(md5_ctx_t *ctx) {
    unsigned int a = ctx->state[0], b = ctx->state[1];
    unsigned int c = ctx->state[2], d = ctx->state[3];
    unsigned int m[16];

    for (int i = 0; i < 16; i++)
        m[i] = (unsigned int)ctx->buffer[i*4] |
               ((unsigned int)ctx->buffer[i*4+1] << 8) |
               ((unsigned int)ctx->buffer[i*4+2] << 16) |
               ((unsigned int)ctx->buffer[i*4+3] << 24);

    for (unsigned int i = 0; i < 64; i++) {
        unsigned int f, g;
        if (i < 16)      { f = MD5_F(b,c,d); g = i; }
        else if (i < 32) { f = MD5_G(b,c,d); g = (5*i+1) % 16; }
        else if (i < 48) { f = MD5_H(b,c,d); g = (3*i+5) % 16; }
        else              { f = MD5_I(b,c,d); g = (7*i) % 16; }

        unsigned int temp = d;
        d = c;
        c = b;
        b = b + MD5_ROT(a + f + md5_k[i] + m[g], md5_r[i]);
        a = temp;
    }

    ctx->state[0] += a; ctx->state[1] += b;
    ctx->state[2] += c; ctx->state[3] += d;
}

static inline void md5_init(md5_ctx_t *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->count = 0;
    mem_set(ctx->buffer, 0, 64);
}

static inline void md5_update(md5_ctx_t *ctx, const unsigned char *data, unsigned long len) {
    for (unsigned long i = 0; i < len; i++) {
        ctx->buffer[ctx->count % 64] = data[i];
        ctx->count++;
        if (ctx->count % 64 == 0)
            md5_transform(ctx);
    }
}

static inline void md5_final(md5_ctx_t *ctx, unsigned char digest[16]) {
    unsigned long bits = ctx->count * 8;
    unsigned long idx = ctx->count % 64;

    ctx->buffer[idx++] = 0x80;
    if (idx > 56) {
        while (idx < 64) ctx->buffer[idx++] = 0;
        md5_transform(ctx);
        idx = 0;
    }
    while (idx < 56) ctx->buffer[idx++] = 0;

    for (int i = 0; i < 8; i++)
        ctx->buffer[56 + i] = (unsigned char)(bits >> (i * 8));

    md5_transform(ctx);

    for (int i = 0; i < 4; i++) {
        digest[i*4]   = (unsigned char)(ctx->state[i]);
        digest[i*4+1] = (unsigned char)(ctx->state[i] >> 8);
        digest[i*4+2] = (unsigned char)(ctx->state[i] >> 16);
        digest[i*4+3] = (unsigned char)(ctx->state[i] >> 24);
    }
}

#endif /* QEMT_MD5_H */

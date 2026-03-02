/*
 * qemt - DEFLATE inflate (decompression) implementation (RFC 1951)
 * No glibc dependency. Used by gunzip, zcat, and unzip tools.
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifndef QEMT_DEFLATE_H
#define QEMT_DEFLATE_H

#include "types.h"
#include "string.h"
#include "syscalls.h"

/* ===== CRC32 (used by gzip verification) ===== */

static inline unsigned int crc32_update(unsigned int crc, unsigned char byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++)
        crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    return crc;
}

static inline unsigned int crc32_buf(const unsigned char *data, unsigned long len) {
    unsigned int crc = 0xFFFFFFFF;
    for (unsigned long i = 0; i < len; i++)
        crc = crc32_update(crc, data[i]);
    return crc ^ 0xFFFFFFFF;
}

/* ===== DEFLATE Constants ===== */

#define INFLATE_MAX_BITS   15
#define INFLATE_MAX_LITS  288
#define INFLATE_MAX_DISTS  32
#define INFLATE_MAX_CL     19
#define INFLATE_WINDOW  32768
#define INFLATE_OUTBUF   8192

/* ===== Bit Reader ===== */

typedef struct {
    const unsigned char *src;
    unsigned long src_len;
    unsigned long pos;
    unsigned int buf;
    int cnt;
} inflate_bits_t;

static inline void inf_bits_init(inflate_bits_t *b, const unsigned char *src, unsigned long len) {
    b->src = src;
    b->src_len = len;
    b->pos = 0;
    b->buf = 0;
    b->cnt = 0;
}

static inline unsigned int inf_bits_read(inflate_bits_t *b, int n) {
    while (b->cnt < n) {
        if (b->pos >= b->src_len) return 0;
        b->buf |= (unsigned int)b->src[b->pos++] << b->cnt;
        b->cnt += 8;
    }
    unsigned int val = b->buf & ((1u << n) - 1);
    b->buf >>= n;
    b->cnt -= n;
    return val;
}

/* ===== Huffman Tree ===== */

typedef struct {
    unsigned short count[INFLATE_MAX_BITS + 1];
    unsigned short symbol[INFLATE_MAX_LITS];
} inflate_huff_t;

static inline int inf_huff_build(inflate_huff_t *h, const unsigned char *lengths, int nsyms) {
    unsigned short offsets[INFLATE_MAX_BITS + 1];
    int i;

    for (i = 0; i <= INFLATE_MAX_BITS; i++) h->count[i] = 0;
    for (i = 0; i < nsyms; i++) h->count[lengths[i]]++;

    if (h->count[0] == (unsigned short)nsyms) return 0;

    offsets[0] = 0;
    for (i = 1; i <= INFLATE_MAX_BITS; i++)
        offsets[i] = offsets[i - 1] + h->count[i - 1];

    for (i = 0; i < nsyms; i++)
        if (lengths[i])
            h->symbol[offsets[lengths[i]]++] = (unsigned short)i;
    return 0;
}

static inline int inf_huff_decode(inflate_bits_t *b, const inflate_huff_t *h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= INFLATE_MAX_BITS; len++) {
        code |= (int)inf_bits_read(b, 1);
        int cnt = h->count[len];
        if (code < first + cnt)
            return h->symbol[index + (code - first)];
        index += cnt;
        first = (first + cnt) << 1;
        code <<= 1;
    }
    return -1;
}

/* ===== DEFLATE Tables ===== */

static const unsigned short inf_len_base[] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
    35,43,51,59,67,83,99,115,131,163,195,227,258
};

static const unsigned char inf_len_extra[] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
    3,3,3,3,4,4,4,4,5,5,5,5,0
};

static const unsigned short inf_dist_base[] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
    257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
};

static const unsigned char inf_dist_extra[] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,11,11,12,12,13,13
};

static const unsigned char inf_cl_order[] = {
    16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
};

/* ===== Streaming Inflate Context ===== */

typedef struct {
    int out_fd;
    unsigned char outbuf[INFLATE_OUTBUF];
    int outpos;
    unsigned char window[INFLATE_WINDOW];
    unsigned long winpos;
    unsigned long total;
    unsigned int crc;
} inflate_out_t;

static inline int inf_flush(inflate_out_t *o) {
    if (o->outpos > 0) {
        const unsigned char *p = o->outbuf;
        int remaining = o->outpos;
        while (remaining > 0) {
            ssize_t w = sys_write(o->out_fd, p, (size_t)remaining);
            if (w <= 0) return -1;
            p += w;
            remaining -= (int)w;
        }
        o->outpos = 0;
    }
    return 0;
}

static inline int inf_emit(inflate_out_t *o, unsigned char byte) {
    o->window[o->winpos++ & (INFLATE_WINDOW - 1)] = byte;
    o->crc = crc32_update(o->crc, byte);
    o->outbuf[o->outpos++] = byte;
    o->total++;
    if (o->outpos >= INFLATE_OUTBUF)
        return inf_flush(o);
    return 0;
}

/* ===== Main Inflate Function ===== */
/*
 * Decompress DEFLATE data from src to out_fd.
 * Returns total decompressed bytes on success, -1 on error.
 */
static inline long inflate_to_fd(const unsigned char *src, unsigned long src_len,
                                  int out_fd) {
    inflate_bits_t bits;
    inflate_out_t out;
    inflate_huff_t lit_tree, dist_tree;
    int bfinal, btype;

    inf_bits_init(&bits, src, src_len);
    mem_set(&out, 0, sizeof(out));
    out.out_fd = out_fd;
    out.crc = 0xFFFFFFFF;

    do {
        bfinal = (int)inf_bits_read(&bits, 1);
        btype = (int)inf_bits_read(&bits, 2);

        if (btype == 0) {
            /* Stored block: align to byte boundary */
            bits.buf = 0;
            bits.cnt = 0;

            if (bits.pos + 4 > bits.src_len) return -1;
            unsigned int len = bits.src[bits.pos] |
                              ((unsigned int)bits.src[bits.pos + 1] << 8);
            bits.pos += 4; /* skip len and nlen */

            if (bits.pos + len > bits.src_len) return -1;
            for (unsigned int i = 0; i < len; i++) {
                if (inf_emit(&out, bits.src[bits.pos++]) < 0) return -1;
            }
        } else if (btype == 1 || btype == 2) {
            if (btype == 1) {
                /* Fixed Huffman codes */
                unsigned char lengths[288];
                int i;
                for (i = 0; i <= 143; i++) lengths[i] = 8;
                for (i = 144; i <= 255; i++) lengths[i] = 9;
                for (i = 256; i <= 279; i++) lengths[i] = 7;
                for (i = 280; i <= 287; i++) lengths[i] = 8;
                inf_huff_build(&lit_tree, lengths, 288);

                unsigned char dlengths[32];
                for (i = 0; i < 32; i++) dlengths[i] = 5;
                inf_huff_build(&dist_tree, dlengths, 32);
            } else {
                /* Dynamic Huffman codes */
                int hlit = (int)inf_bits_read(&bits, 5) + 257;
                int hdist = (int)inf_bits_read(&bits, 5) + 1;
                int hclen = (int)inf_bits_read(&bits, 4) + 4;

                unsigned char cl_lengths[INFLATE_MAX_CL];
                mem_set(cl_lengths, 0, sizeof(cl_lengths));
                for (int i = 0; i < hclen; i++)
                    cl_lengths[inf_cl_order[i]] = (unsigned char)inf_bits_read(&bits, 3);

                inflate_huff_t cl_tree;
                inf_huff_build(&cl_tree, cl_lengths, INFLATE_MAX_CL);

                unsigned char all_lengths[INFLATE_MAX_LITS + INFLATE_MAX_DISTS];
                mem_set(all_lengths, 0, sizeof(all_lengths));
                int total = hlit + hdist;
                int idx = 0;

                while (idx < total) {
                    int sym = inf_huff_decode(&bits, &cl_tree);
                    if (sym < 0) return -1;

                    if (sym < 16) {
                        all_lengths[idx++] = (unsigned char)sym;
                    } else if (sym == 16) {
                        int rep = (int)inf_bits_read(&bits, 2) + 3;
                        if (idx == 0) return -1;
                        unsigned char prev = all_lengths[idx - 1];
                        while (rep-- > 0 && idx < total)
                            all_lengths[idx++] = prev;
                    } else if (sym == 17) {
                        int rep = (int)inf_bits_read(&bits, 3) + 3;
                        while (rep-- > 0 && idx < total)
                            all_lengths[idx++] = 0;
                    } else if (sym == 18) {
                        int rep = (int)inf_bits_read(&bits, 7) + 11;
                        while (rep-- > 0 && idx < total)
                            all_lengths[idx++] = 0;
                    } else {
                        return -1;
                    }
                }

                inf_huff_build(&lit_tree, all_lengths, hlit);
                inf_huff_build(&dist_tree, all_lengths + hlit, hdist);
            }

            /* Decode compressed data */
            for (;;) {
                int sym = inf_huff_decode(&bits, &lit_tree);
                if (sym < 0) return -1;

                if (sym < 256) {
                    if (inf_emit(&out, (unsigned char)sym) < 0) return -1;
                } else if (sym == 256) {
                    break;
                } else {
                    /* Length-distance pair */
                    int len_idx = sym - 257;
                    if (len_idx < 0 || len_idx > 28) return -1;
                    int length = inf_len_base[len_idx] +
                                (int)inf_bits_read(&bits, inf_len_extra[len_idx]);

                    int dist_sym = inf_huff_decode(&bits, &dist_tree);
                    if (dist_sym < 0 || dist_sym > 29) return -1;
                    int distance = inf_dist_base[dist_sym] +
                                  (int)inf_bits_read(&bits, inf_dist_extra[dist_sym]);

                    for (int i = 0; i < length; i++) {
                        unsigned char byte = out.window[
                            (out.winpos - (unsigned long)distance) &
                            (INFLATE_WINDOW - 1)];
                        if (inf_emit(&out, byte) < 0) return -1;
                    }
                }
            }
        } else {
            return -1; /* invalid block type */
        }
    } while (!bfinal);

    /* Final flush */
    if (inf_flush(&out) < 0) return -1;

    return (long)out.total;
}

/* Get final CRC32 after inflate_to_fd (call before next inflate) */
static inline unsigned int inflate_get_crc(const inflate_out_t *o) {
    return o->crc ^ 0xFFFFFFFF;
}

/* ===== Gzip Header Parser ===== */

#define GZIP_MAGIC1 0x1f
#define GZIP_MAGIC2 0x8b
#define GZIP_METHOD_DEFLATE 8

#define GZIP_FTEXT    0x01
#define GZIP_FHCRC    0x02
#define GZIP_FEXTRA   0x04
#define GZIP_FNAME    0x08
#define GZIP_FCOMMENT 0x10

typedef struct {
    unsigned long data_offset; /* offset to DEFLATE data */
    unsigned long data_len;    /* length of DEFLATE data */
    unsigned int  orig_crc;    /* stored CRC32 */
    unsigned int  orig_size;   /* stored original size (mod 2^32) */
} gzip_header_t;

/*
 * Parse gzip header from mapped file data.
 * Returns 0 on success, -1 on error.
 */
static inline int gzip_parse_header(const unsigned char *data, unsigned long len,
                                     gzip_header_t *hdr) {
    if (len < 18) return -1;
    if (data[0] != GZIP_MAGIC1 || data[1] != GZIP_MAGIC2) return -1;
    if (data[2] != GZIP_METHOD_DEFLATE) return -1;

    unsigned char flags = data[3];
    unsigned long pos = 10; /* skip fixed header */

    /* FEXTRA */
    if (flags & GZIP_FEXTRA) {
        if (pos + 2 > len) return -1;
        unsigned int xlen = data[pos] | ((unsigned int)data[pos + 1] << 8);
        pos += 2 + xlen;
    }

    /* FNAME */
    if (flags & GZIP_FNAME) {
        while (pos < len && data[pos] != 0) pos++;
        pos++; /* skip null terminator */
    }

    /* FCOMMENT */
    if (flags & GZIP_FCOMMENT) {
        while (pos < len && data[pos] != 0) pos++;
        pos++;
    }

    /* FHCRC */
    if (flags & GZIP_FHCRC) {
        pos += 2;
    }

    if (pos >= len) return -1;

    hdr->data_offset = pos;
    /* CRC32 and ISIZE are the last 8 bytes */
    hdr->orig_crc = data[len - 8] | ((unsigned int)data[len - 7] << 8) |
                    ((unsigned int)data[len - 6] << 16) | ((unsigned int)data[len - 5] << 24);
    hdr->orig_size = data[len - 4] | ((unsigned int)data[len - 3] << 8) |
                     ((unsigned int)data[len - 2] << 16) | ((unsigned int)data[len - 1] << 24);
    hdr->data_len = len - pos - 8;

    return 0;
}

#endif /* QEMT_DEFLATE_H */

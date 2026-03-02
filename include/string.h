/*
 * qemt - String utility functions (no glibc dependency)
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ---
 *
 * Basic string manipulation using only raw operations.
 */

#ifndef QEMT_STRING_H
#define QEMT_STRING_H

#include "types.h"

static inline size_t str_len(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static inline int str_cmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static inline int str_ncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *b && *a == *b) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

static inline char *str_cpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

static inline char *str_ncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

static inline char *str_cat(char *dest, const char *src) {
    char *d = dest + str_len(dest);
    while ((*d++ = *src++));
    return dest;
}

static inline char *str_chr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return NULL;
}

static inline char *str_rchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return (char *)last;
}

static inline char *str_str(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}

static inline void *mem_set(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

static inline void *mem_cpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}

static inline void *mem_move(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

static inline int mem_cmp(const void *a, const void *b, size_t n) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    while (n--) {
        if (*pa != *pb) return *pa - *pb;
        pa++; pb++;
    }
    return 0;
}

/* Number to string conversions */
static inline int uint_to_str(char *buf, unsigned int val) {
    char tmp[12];
    int i = 0;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int len = i;
    for (int j = 0; j < len; j++) buf[j] = tmp[len - 1 - j];
    buf[len] = '\0';
    return len;
}

static inline int int_to_str(char *buf, int val) {
    if (val < 0) {
        buf[0] = '-';
        return 1 + uint_to_str(buf + 1, (unsigned int)(-(long)val));
    }
    return uint_to_str(buf, (unsigned int)val);
}

static inline int ulong_to_str(char *buf, unsigned long val) {
    char tmp[21];
    int i = 0;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int len = i;
    for (int j = 0; j < len; j++) buf[j] = tmp[len - 1 - j];
    buf[len] = '\0';
    return len;
}

static inline int long_to_str(char *buf, long val) {
    if (val < 0) {
        buf[0] = '-';
        return 1 + ulong_to_str(buf + 1, (unsigned long)(-val));
    }
    return ulong_to_str(buf, (unsigned long)val);
}

/* Octal conversion */
static inline int uint_to_oct(char *buf, unsigned int val) {
    char tmp[16];
    int i = 0;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    while (val > 0) { tmp[i++] = '0' + (val & 7); val >>= 3; }
    int len = i;
    for (int j = 0; j < len; j++) buf[j] = tmp[len - 1 - j];
    buf[len] = '\0';
    return len;
}

/* Parse unsigned int from string */
static inline unsigned int str_to_uint(const char *s) {
    unsigned int val = 0;
    while (*s >= '0' && *s <= '9') { val = val * 10 + (*s - '0'); s++; }
    return val;
}

static inline long str_to_long(const char *s) {
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    long val = 0;
    while (*s >= '0' && *s <= '9') { val = val * 10 + (*s - '0'); s++; }
    return neg ? -val : val;
}

/* Parse octal from string */
static inline unsigned int oct_to_uint(const char *s) {
    unsigned int val = 0;
    while (*s >= '0' && *s <= '7') { val = (val << 3) | (*s - '0'); s++; }
    return val;
}

/* Character classification */
static inline int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static inline int is_digit(char c) { return c >= '0' && c <= '9'; }
static inline int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static inline int is_alnum(char c) { return is_alpha(c) || is_digit(c); }
static inline int is_upper(char c) { return c >= 'A' && c <= 'Z'; }
static inline int is_lower(char c) { return c >= 'a' && c <= 'z'; }
static inline char to_lower(char c) { return is_upper(c) ? c + 32 : c; }
static inline char to_upper(char c) { return is_lower(c) ? c - 32 : c; }

static inline const char *skip_space(const char *s) {
    while (*s && is_space(*s)) s++;
    return s;
}

static inline int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return 1;
}

static inline int ends_with(const char *s, const char *suffix) {
    size_t sl = str_len(s), xl = str_len(suffix);
    if (xl > sl) return 0;
    return str_cmp(s + sl - xl, suffix) == 0;
}

/* Hex conversion */
static inline int ulong_to_hex(char *buf, unsigned long val, int upper) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[17];
    int i = 0;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    while (val > 0) { tmp[i++] = digits[val & 0xf]; val >>= 4; }
    int len = i;
    for (int j = 0; j < len; j++) buf[j] = tmp[len - 1 - j];
    buf[len] = '\0';
    return len;
}

static inline unsigned long hex_to_ulong(const char *s) {
    unsigned long val = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (*s) {
        char c = *s;
        if (c >= '0' && c <= '9') val = (val << 4) | (unsigned long)(c - '0');
        else if (c >= 'a' && c <= 'f') val = (val << 4) | (unsigned long)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val = (val << 4) | (unsigned long)(c - 'A' + 10);
        else break;
        s++;
    }
    return val;
}

#endif /* QEMT_STRING_H */

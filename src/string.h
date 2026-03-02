/*
 * qoot - String utility functions (no glibc dependency)
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

#ifndef QOOT_STRING_H
#define QOOT_STRING_H

#include "syscalls.h"

/* String length */
static inline size_t str_len(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

/* String compare */
static inline int str_cmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

/* String compare with length limit */
static inline int str_ncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *b && *a == *b) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

/* Copy string, returns pointer to dest */
static inline char *str_cpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

/* Copy at most n chars */
static inline char *str_ncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

/* Concatenate strings */
static inline char *str_cat(char *dest, const char *src) {
    char *d = dest + str_len(dest);
    while ((*d++ = *src++));
    return dest;
}

/* Find character in string */
static inline char *str_chr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return NULL;
}

/* Memory set */
static inline void *mem_set(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

/* Memory copy */
static inline void *mem_cpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}

/* Convert unsigned int to decimal string, returns length */
static inline int uint_to_str(char *buf, unsigned int val) {
    char tmp[12];
    int i = 0;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int len = i;
    while (i > 0) {
        buf[len - i] = tmp[i - 1];
        i--;
    }
    buf[len] = '\0';
    return len;
}

/* Parse unsigned int from string */
static inline unsigned int str_to_uint(const char *s) {
    unsigned int val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val;
}

/* Write string to fd */
static inline void write_str(int fd, const char *s) {
    sys_write(fd, s, str_len(s));
}

/* Write string to stdout */
static inline void print(const char *s) {
    write_str(1, s);
}

/* Write string to stderr */
static inline void eprint(const char *s) {
    write_str(2, s);
}

/* Check if character is whitespace */
static inline int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* Skip leading whitespace */
static inline const char *skip_space(const char *s) {
    while (*s && is_space(*s)) s++;
    return s;
}

/* Check if string starts with prefix */
static inline int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

#endif /* QOOT_STRING_H */

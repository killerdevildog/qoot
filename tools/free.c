/*
 * free - display memory usage (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static void pad_left(const char *s, int w) {
    int l = (int)str_len(s);
    while (l++ < w) print(" ");
    print(s);
}

static unsigned long extract_meminfo_val(const char *buf, const char *key) {
    const char *p = str_str(buf, key);
    if (!p) return 0;
    p += str_len(key);
    while (*p == ' ' || *p == ':') p++;
    return str_to_uint(p); /* Value is in kB */
}

int free_main(int argc, char **argv, char **envp) {
    (void)envp;
    int human = 0;
    if (argc > 1 && str_cmp(argv[1], "-h") == 0) human = 1;

    int fd = sys_open("/proc/meminfo", O_RDONLY, 0);
    if (fd < 0) { eprint("free: cannot read /proc/meminfo\n"); return 1; }

    char buf[4096];
    ssize_t n = sys_read(fd, buf, sizeof(buf) - 1);
    sys_close(fd);
    if (n <= 0) return 1;
    buf[n] = '\0';

    unsigned long total = extract_meminfo_val(buf, "MemTotal");
    unsigned long free_mem = extract_meminfo_val(buf, "MemFree");
    unsigned long avail = extract_meminfo_val(buf, "MemAvailable");
    unsigned long buffers = extract_meminfo_val(buf, "Buffers");
    unsigned long cached = extract_meminfo_val(buf, "Cached");
    unsigned long swap_total = extract_meminfo_val(buf, "SwapTotal");
    unsigned long swap_free = extract_meminfo_val(buf, "SwapFree");
    unsigned long shared = extract_meminfo_val(buf, "Shmem");

    unsigned long used = total - free_mem - buffers - cached;
    unsigned long swap_used = swap_total - swap_free;

    char nbuf[21];
    int w = human ? 8 : 12;

    /* Header */
    print("              ");
    pad_left("total", w); pad_left("used", w); pad_left("free", w);
    pad_left("shared", w); pad_left("buff/cache", w); pad_left("available", w);
    print("\n");

    /* Mem */
    print("Mem:          ");
    if (human) {
        /* Convert kB to human readable */
        const char *suffixes[] = {"K", "M", "G", "T"};
        unsigned long vals[] = {total, used, free_mem, shared, buffers + cached, avail};
        for (int i = 0; i < 6; i++) {
            unsigned long v = vals[i];
            int si = 0;
            while (v >= 1024 && si < 3) { v /= 1024; si++; }
            ulong_to_str(nbuf, v);
            str_cat(nbuf, suffixes[si]);
            pad_left(nbuf, w);
        }
    } else {
        ulong_to_str(nbuf, total); pad_left(nbuf, w);
        ulong_to_str(nbuf, used); pad_left(nbuf, w);
        ulong_to_str(nbuf, free_mem); pad_left(nbuf, w);
        ulong_to_str(nbuf, shared); pad_left(nbuf, w);
        ulong_to_str(nbuf, buffers + cached); pad_left(nbuf, w);
        ulong_to_str(nbuf, avail); pad_left(nbuf, w);
    }
    print("\n");

    /* Swap */
    print("Swap:         ");
    if (human) {
        const char *suffixes[] = {"K", "M", "G", "T"};
        unsigned long vals[] = {swap_total, swap_used, swap_free};
        for (int i = 0; i < 3; i++) {
            unsigned long v = vals[i];
            int si = 0;
            while (v >= 1024 && si < 3) { v /= 1024; si++; }
            ulong_to_str(nbuf, v);
            str_cat(nbuf, suffixes[si]);
            pad_left(nbuf, w);
        }
    } else {
        ulong_to_str(nbuf, swap_total); pad_left(nbuf, w);
        ulong_to_str(nbuf, swap_used); pad_left(nbuf, w);
        ulong_to_str(nbuf, swap_free); pad_left(nbuf, w);
    }
    print("\n");

    return 0;
}

QEMT_ENTRY(free_main)

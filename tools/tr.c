/*
 * tr - translate characters (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int tr_main(int argc, char **argv, char **envp) {
    (void)envp;
    int delete_mode = 0;
    int first = 1;

    if (first < argc && str_cmp(argv[first], "-d") == 0) { delete_mode = 1; first++; }

    if (first >= argc) { eprint("Usage: tr [-d] set1 [set2]\n"); return 1; }

    const char *set1 = argv[first++];
    const char *set2 = (first < argc) ? argv[first] : NULL;

    /* Build translation table */
    unsigned char table[256];
    unsigned char del_set[256];
    mem_set(del_set, 0, sizeof(del_set));
    for (int i = 0; i < 256; i++) table[i] = (unsigned char)i;

    if (delete_mode) {
        for (int i = 0; set1[i]; i++) del_set[(unsigned char)set1[i]] = 1;
    } else if (set2) {
        size_t l1 = str_len(set1), l2 = str_len(set2);
        for (size_t i = 0; i < l1; i++) {
            size_t j = (i < l2) ? i : l2 - 1;
            table[(unsigned char)set1[i]] = (unsigned char)set2[j];
        }
    }

    char buf[4096];
    ssize_t n;
    while ((n = sys_read(0, buf, sizeof(buf))) > 0) {
        if (delete_mode) {
            for (ssize_t i = 0; i < n; i++) {
                if (!del_set[(unsigned char)buf[i]]) {
                    char c = buf[i];
                    sys_write(1, &c, 1);
                }
            }
        } else {
            for (ssize_t i = 0; i < n; i++)
                buf[i] = (char)table[(unsigned char)buf[i]];
            sys_write(1, buf, (size_t)n);
        }
    }
    return 0;
}

QEMT_ENTRY(tr_main)

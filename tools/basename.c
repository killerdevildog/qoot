/*
 * basename - strip directory from filename (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int basename_main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc < 2) { eprint("Usage: basename path [suffix]\n"); return 1; }

    char *path = argv[1];
    /* Remove trailing slashes */
    size_t len = str_len(path);
    while (len > 1 && path[len - 1] == '/') path[--len] = '\0';

    const char *base = str_rchr(path, '/');
    base = base ? base + 1 : path;

    char result[PATH_MAX];
    str_cpy(result, base);

    /* Remove suffix if given */
    if (argc > 2) {
        size_t rlen = str_len(result);
        size_t slen = str_len(argv[2]);
        if (rlen > slen && str_cmp(result + rlen - slen, argv[2]) == 0) {
            result[rlen - slen] = '\0';
        }
    }

    print(result); print("\n");
    return 0;
}

QEMT_ENTRY(basename_main)

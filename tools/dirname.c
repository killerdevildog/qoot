/*
 * dirname - strip last component from path (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int dirname_main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc < 2) { eprint("Usage: dirname path\n"); return 1; }

    char path[PATH_MAX];
    str_ncpy(path, argv[1], sizeof(path) - 1);
    size_t len = str_len(path);

    /* Remove trailing slashes */
    while (len > 1 && path[len - 1] == '/') path[--len] = '\0';

    /* Find last slash */
    char *slash = str_rchr(path, '/');
    if (!slash) {
        print(".\n");
    } else if (slash == path) {
        print("/\n");
    } else {
        *slash = '\0';
        print(path); print("\n");
    }
    return 0;
}

QEMT_ENTRY(dirname_main)

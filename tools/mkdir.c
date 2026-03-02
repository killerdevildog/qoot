/*
 * mkdir - create directories (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int mkdir_parents(const char *path, int mode) {
    char tmp[PATH_MAX];
    str_cpy(tmp, path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            sys_mkdir(tmp, mode); /* Ignore errors for existing dirs */
            *p = '/';
        }
    }
    return sys_mkdir(tmp, mode);
}

int mkdir_main(int argc, char **argv, char **envp) {
    (void)envp;
    int parents = 0;
    int mode = 0755;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: mkdir [-p] [-m mode] directory...\n");
        print("  -p  Create parent directories as needed\n");
        print("  -m  Set permission mode (octal)\n");
        return 0;
    }

    while (first < argc && argv[first][0] == '-') {
        if (str_cmp(argv[first], "-p") == 0) { parents = 1; first++; }
        else if (str_cmp(argv[first], "-m") == 0 && first + 1 < argc) {
            mode = (int)oct_to_uint(argv[first + 1]);
            first += 2;
        } else { first++; }
    }

    if (first >= argc) {
        eprint("mkdir: missing operand\n");
        return 1;
    }

    int ret = 0;
    for (int i = first; i < argc; i++) {
        int r;
        if (parents) r = mkdir_parents(argv[i], mode);
        else r = sys_mkdir(argv[i], mode);

        if (r < 0 && !(parents && r == -17)) { /* -17 = EEXIST with -p is OK */
            eprint("mkdir: cannot create '"); eprint(argv[i]); eprint("'\n");
            ret = 1;
        }
    }
    return ret;
}

QEMT_ENTRY(mkdir_main)

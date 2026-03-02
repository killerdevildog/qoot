/*
 * readlink - print resolved symbolic link (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int readlink_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc < 2 || str_cmp(argv[1], "-h") == 0) {
        print("Usage: readlink [-f] link...\n");
        return argc < 2 ? 1 : 0;
    }

    int first = 1;
    if (str_cmp(argv[1], "-f") == 0) first = 2; /* -f canonicalize (simplified) */

    int ret = 0;
    for (int i = first; i < argc; i++) {
        char buf[PATH_MAX];
        ssize_t n = sys_readlink(argv[i], buf, sizeof(buf) - 1);
        if (n < 0) {
            eprint("readlink: "); eprint(argv[i]); eprint(": Not a symlink\n");
            ret = 1;
            continue;
        }
        buf[n] = '\0';
        print(buf);
        print("\n");
    }
    return ret;
}

QEMT_ENTRY(readlink_main)

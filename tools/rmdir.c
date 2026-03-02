/*
 * rmdir - remove empty directories (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int rmdir_main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc < 2) { eprint("rmdir: missing operand\n"); return 1; }
    int ret = 0;
    for (int i = 1; i < argc; i++) {
        if (sys_rmdir(argv[i]) < 0) {
            eprint("rmdir: failed to remove '"); eprint(argv[i]); eprint("'\n");
            ret = 1;
        }
    }
    return ret;
}

QEMT_ENTRY(rmdir_main)

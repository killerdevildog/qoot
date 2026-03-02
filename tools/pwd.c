/*
 * pwd - print working directory (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int pwd_main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;
    char buf[PATH_MAX];
    if (sys_getcwd(buf, sizeof(buf)) < 0) {
        eprint("pwd: cannot get current directory\n");
        return 1;
    }
    print(buf);
    print("\n");
    return 0;
}

QEMT_ENTRY(pwd_main)

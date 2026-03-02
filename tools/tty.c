/*
 * tty - print terminal name (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int tty_main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;

    /* Read /proc/self/fd/0 symlink to find terminal */
    char buf[PATH_MAX];
    ssize_t n = sys_readlink("/proc/self/fd/0", buf, PATH_MAX - 1);
    if (n > 0) {
        buf[n] = '\0';
        print(buf);
        print("\n");
        return 0;
    }

    print("not a tty\n");
    return 1;
}

QEMT_ENTRY(tty_main)

/*
 * yes - output string repeatedly (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int yes_main(int argc, char **argv, char **envp) {
    (void)envp;
    const char *msg = "y";

    if (argc > 1) msg = argv[1];

    /* Buffer for efficiency */
    char buf[4096];
    int pos = 0;
    size_t mlen = str_len(msg);

    for (;;) {
        if (pos + (int)mlen + 1 >= (int)sizeof(buf)) {
            if (sys_write(1, buf, (size_t)pos) <= 0) break;
            pos = 0;
        }
        mem_cpy(buf + pos, msg, mlen);
        pos += (int)mlen;
        buf[pos++] = '\n';
    }
    return 0;
}

QEMT_ENTRY(yes_main)

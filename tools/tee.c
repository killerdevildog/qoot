/*
 * tee - read stdin and write to stdout and files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int tee_main(int argc, char **argv, char **envp) {
    (void)envp;
    int append = 0, first = 1;

    if (argc > 1 && str_cmp(argv[1], "-a") == 0) { append = 1; first = 2; }

    int fds[32];
    int nfds = 0;

    for (int i = first; i < argc && nfds < 32; i++) {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        int fd = sys_open(argv[i], flags, 0644);
        if (fd < 0) { eprint("tee: cannot open '"); eprint(argv[i]); eprint("'\n"); continue; }
        fds[nfds++] = fd;
    }

    char buf[4096];
    ssize_t n;
    while ((n = sys_read(0, buf, sizeof(buf))) > 0) {
        sys_write(1, buf, (size_t)n);
        for (int i = 0; i < nfds; i++)
            sys_write(fds[i], buf, (size_t)n);
    }

    for (int i = 0; i < nfds; i++) sys_close(fds[i]);
    return 0;
}

QEMT_ENTRY(tee_main)

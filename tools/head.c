/*
 * head - output first part of files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int head_main(int argc, char **argv, char **envp) {
    (void)envp;
    int nlines = 10;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: head [-n lines] [file...]\n");
        return 0;
    }

    if (argc > 2 && str_cmp(argv[1], "-n") == 0) {
        nlines = (int)str_to_uint(argv[2]);
        first = 3;
    } else if (argc > 1 && argv[1][0] == '-' && is_digit(argv[1][1])) {
        nlines = (int)str_to_uint(argv[1] + 1);
        first = 2;
    }

    int nfiles = argc - first;
    if (nfiles == 0) {
        /* Read from stdin */
        char buf[4096];
        int lines = 0;
        while (lines < nlines) {
            ssize_t n = sys_read(0, buf, sizeof(buf));
            if (n <= 0) break;
            for (ssize_t j = 0; j < n && lines < nlines; j++) {
                char c[2] = {buf[j], '\0'};
                print(c);
                if (buf[j] == '\n') lines++;
            }
        }
        return 0;
    }

    for (int i = first; i < argc; i++) {
        int fd = sys_open(argv[i], O_RDONLY, 0);
        if (fd < 0) {
            eprint("head: cannot open '"); eprint(argv[i]); eprint("'\n");
            continue;
        }

        if (nfiles > 1) {
            print("==> "); print(argv[i]); print(" <==\n");
        }

        char buf[4096];
        int lines = 0;
        while (lines < nlines) {
            ssize_t n = sys_read(fd, buf, sizeof(buf));
            if (n <= 0) break;
            for (ssize_t j = 0; j < n && lines < nlines; j++) {
                char c[2] = {buf[j], '\0'};
                print(c);
                if (buf[j] == '\n') lines++;
            }
        }
        sys_close(fd);
        if (i < argc - 1) print("\n");
    }
    return 0;
}

QEMT_ENTRY(head_main)

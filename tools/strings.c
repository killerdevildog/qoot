/*
 * strings - extract printable strings from binary files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int is_printable(unsigned char c) {
    return (c >= 32 && c <= 126) || c == '\t';
}

static void strings_fd(int fd, int min_len) {
    unsigned char buf[4096];
    char str[4096];
    int slen = 0;
    ssize_t r;

    while ((r = sys_read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < r; i++) {
            if (is_printable(buf[i])) {
                if (slen < (int)sizeof(str) - 1)
                    str[slen++] = (char)buf[i];
            } else {
                if (slen >= min_len) {
                    str[slen] = '\0';
                    print(str);
                    print("\n");
                }
                slen = 0;
            }
        }
    }

    /* Flush remaining */
    if (slen >= min_len) {
        str[slen] = '\0';
        print(str);
        print("\n");
    }
}

int strings_main(int argc, char **argv, char **envp) {
    (void)envp;
    int min_len = 4;
    const char *files[64];
    int nfiles = 0;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-n") == 0 && i + 1 < argc) {
            min_len = (int)str_to_uint(argv[++i]);
        } else if (starts_with(argv[i], "-n")) {
            min_len = (int)str_to_uint(argv[i] + 2);
        } else if (str_cmp(argv[i], "-h") == 0 || str_cmp(argv[i], "--help") == 0) {
            print("Usage: strings [-n MIN] [file...]\n");
            print("  -n MIN  Minimum string length (default: 4)\n");
            return 0;
        } else if (argv[i][0] != '-') {
            if (nfiles < 64) files[nfiles++] = argv[i];
        }
    }

    if (min_len < 1) min_len = 1;

    if (nfiles == 0) {
        strings_fd(0, min_len);
    } else {
        for (int i = 0; i < nfiles; i++) {
            int fd = sys_open(files[i], O_RDONLY, 0);
            if (fd < 0) {
                eprint("strings: "); eprint(files[i]); eprint(": cannot open\n");
                continue;
            }
            strings_fd(fd, min_len);
            sys_close(fd);
        }
    }
    return 0;
}

QEMT_ENTRY(strings_main)

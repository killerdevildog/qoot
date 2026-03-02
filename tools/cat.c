/*
 * cat - concatenate and print files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int cat_main(int argc, char **argv, char **envp) {
    (void)envp;
    int number = 0;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: cat [-n] [file...]\n");
        print("  -n  Number output lines\n");
        print("  If no file, reads stdin.\n");
        return 0;
    }

    int first_file = 1;
    if (argc > 1 && str_cmp(argv[1], "-n") == 0) {
        number = 1;
        first_file = 2;
    }

    int files = 0;
    unsigned long line_num = 1;

    for (int i = first_file; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '\0') continue; /* stdin marker */
        int fd = sys_open(argv[i], O_RDONLY, 0);
        if (fd < 0) {
            eprint("cat: "); eprint(argv[i]); eprint(": No such file or directory\n");
            continue;
        }
        files++;

        char buf[4096];
        ssize_t n;
        int at_line_start = 1;
        while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
            if (!number) {
                sys_write(1, buf, n);
            } else {
                for (ssize_t j = 0; j < n; j++) {
                    if (at_line_start) {
                        char num[21];
                        ulong_to_str(num, line_num++);
                        int len = (int)str_len(num);
                        for (int k = 0; k < 6 - len; k++) print(" ");
                        print(num);
                        print("\t");
                    }
                    char c[2] = {buf[j], '\0'};
                    print(c);
                    at_line_start = (buf[j] == '\n');
                }
            }
        }
        sys_close(fd);
    }

    /* Read from stdin if no files given */
    if (files == 0 && first_file >= argc) {
        char buf[4096];
        ssize_t n;
        int at_line_start = 1;
        while ((n = sys_read(0, buf, sizeof(buf))) > 0) {
            if (!number) {
                sys_write(1, buf, n);
            } else {
                for (ssize_t j = 0; j < n; j++) {
                    if (at_line_start) {
                        char num[21];
                        ulong_to_str(num, line_num++);
                        int len = (int)str_len(num);
                        for (int k = 0; k < 6 - len; k++) print(" ");
                        print(num);
                        print("\t");
                    }
                    char c[2] = {buf[j], '\0'};
                    print(c);
                    at_line_start = (buf[j] == '\n');
                }
            }
        }
    }

    return 0;
}

QEMT_ENTRY(cat_main)

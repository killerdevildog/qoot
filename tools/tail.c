/*
 * tail - output last part of files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int tail_main(int argc, char **argv, char **envp) {
    (void)envp;
    int nlines = 10;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: tail [-n lines] [file...]\n");
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

    for (int i = (nfiles == 0 ? 0 : first); i < (nfiles == 0 ? 1 : argc); i++) {
        int fd;
        if (nfiles == 0) {
            fd = 0; /* stdin */
        } else {
            fd = sys_open(argv[i], O_RDONLY, 0);
            if (fd < 0) {
                eprint("tail: cannot open '"); eprint(argv[i]); eprint("'\n");
                continue;
            }
            if (nfiles > 1) {
                print("==> "); print(argv[i]); print(" <==\n");
            }
        }

        /* Read entire file into a ring buffer approach: count total lines first */
        /* For files, seek to end and work backwards */
        if (fd > 0) {
            struct stat st;
            if (sys_fstat(fd, &st) == 0 && st.st_size > 0) {
                /* Read file, find last N lines */
                /* Simple approach: read entire file, find line positions */
                long fsize = st.st_size;
                long offset = fsize > 65536 ? fsize - 65536 : 0;
                sys_lseek(fd, offset, 0); /* SEEK_SET */

                char buf[65536];
                ssize_t total = 0;
                ssize_t n;
                while ((n = sys_read(fd, buf + total, sizeof(buf) - (size_t)total)) > 0)
                    total += n;

                /* Count newlines from end */
                int count = 0;
                ssize_t start = total;
                for (ssize_t j = total - 1; j >= 0; j--) {
                    if (buf[j] == '\n') {
                        count++;
                        if (count > nlines) { start = j + 1; break; }
                    }
                }
                if (count <= nlines) start = 0;

                sys_write(1, buf + start, total - start);
                sys_close(fd);
            }
        } else {
            /* Stdin: buffer lines and print last N */
            /* Use a circular buffer of line start positions */
            static char sbuf[65536];
            ssize_t total = 0;
            ssize_t n;
            while ((n = sys_read(fd, sbuf + total, sizeof(sbuf) - (size_t)total)) > 0)
                total += n;

            int count = 0;
            ssize_t start = total;
            for (ssize_t j = total - 1; j >= 0; j--) {
                if (sbuf[j] == '\n') {
                    count++;
                    if (count > nlines) { start = j + 1; break; }
                }
            }
            if (count <= nlines) start = 0;
            sys_write(1, sbuf + start, total - start);
        }

        if (nfiles > 1 && i < argc - 1) print("\n");
    }
    return 0;
}

QEMT_ENTRY(tail_main)

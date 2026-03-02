/*
 * paste - merge lines of files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

#define MAX_FILES 16

int paste_main(int argc, char **argv, char **envp) {
    (void)envp;
    const char *delim = "\t";
    int serial = 0;
    int file_start = 1;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-s") == 0) {
            serial = 1;
            file_start = i + 1;
        } else if (str_cmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delim = argv[++i];
            file_start = i + 1;
        } else if (starts_with(argv[i], "-d")) {
            delim = argv[i] + 2;
            file_start = i + 1;
        } else if (str_cmp(argv[i], "--help") == 0 || str_cmp(argv[i], "-h") == 0) {
            print("Usage: paste [-s] [-d DELIM] FILE...\n");
            print("Merge corresponding lines of files.\n  -d DELIM  Use DELIM instead of tab\n  -s        Paste one file at a time\n  -         Read from stdin\n");
            return 0;
        } else {
            break;
        }
    }

    int nfiles = argc - file_start;
    if (nfiles <= 0) {
        /* Read from stdin, just pass through */
        char buf[4096];
        ssize_t n;
        while ((n = sys_read(0, buf, sizeof(buf))) > 0)
            sys_write(1, buf, n);
        return 0;
    }

    size_t dlen = str_len(delim);
    if (dlen == 0) { delim = "\t"; dlen = 1; }

    int fds[MAX_FILES];
    int actual = nfiles < MAX_FILES ? nfiles : MAX_FILES;

    for (int i = 0; i < actual; i++) {
        const char *name = argv[file_start + i];
        if (str_cmp(name, "-") == 0)
            fds[i] = 0;
        else {
            fds[i] = sys_open(name, O_RDONLY, 0);
            if (fds[i] < 0) {
                eprint("paste: "); eprint(name); eprint(": cannot open\n");
                return 1;
            }
        }
    }

    if (serial) {
        /* Serial mode: each file on one line */
        for (int f = 0; f < actual; f++) {
            char buf[4096];
            char line[4096];
            int lpos = 0;
            int first = 1;
            ssize_t n;

            while ((n = sys_read(fds[f], buf, sizeof(buf))) > 0) {
                for (ssize_t i = 0; i < n; i++) {
                    if (buf[i] == '\n') {
                        if (!first) {
                            char d = delim[(first ? 0 : (first - 1)) % dlen];
                            (void)d;
                            sys_write(1, &delim[(first - 1) % dlen], 1);
                        }
                        sys_write(1, line, lpos);
                        lpos = 0;
                        first++;
                    } else if (lpos < (int)sizeof(line) - 1) {
                        line[lpos++] = buf[i];
                    }
                }
            }
            if (lpos > 0) {
                if (!first) sys_write(1, &delim[(first - 1) % dlen], 1);
                sys_write(1, line, lpos);
            }
            sys_write(1, "\n", 1);
            if (fds[f] > 0) sys_close(fds[f]);
        }
    } else {
        /* Normal mode: merge corresponding lines */
        char bufs[MAX_FILES][4096];
        int bpos[MAX_FILES];
        int blen[MAX_FILES];
        int done[MAX_FILES];

        mem_set(bpos, 0, sizeof(bpos));
        mem_set(blen, 0, sizeof(blen));
        mem_set(done, 0, sizeof(done));

        for (;;) {
            int all_done = 1;
            for (int f = 0; f < actual; f++) {
                if (!done[f]) { all_done = 0; break; }
            }
            if (all_done) break;

            for (int f = 0; f < actual; f++) {
                if (f > 0) {
                    char d = delim[(f - 1) % dlen];
                    sys_write(1, &d, 1);
                }

                if (done[f]) continue;

                /* Read a line from file f */
                for (;;) {
                    /* Scan buffer for newline */
                    while (bpos[f] < blen[f]) {
                        if (bufs[f][bpos[f]] == '\n') {
                            bpos[f]++;
                            goto next_file;
                        }
                        sys_write(1, &bufs[f][bpos[f]], 1);
                        bpos[f]++;
                    }
                    /* Refill buffer */
                    ssize_t n = sys_read(fds[f], bufs[f], sizeof(bufs[f]));
                    if (n <= 0) {
                        done[f] = 1;
                        goto next_file;
                    }
                    bpos[f] = 0;
                    blen[f] = n;
                }
                next_file:;
            }
            sys_write(1, "\n", 1);
        }

        for (int f = 0; f < actual; f++)
            if (fds[f] > 0) sys_close(fds[f]);
    }

    return 0;
}

QEMT_ENTRY(paste_main)

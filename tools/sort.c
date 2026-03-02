/*
 * sort - sort lines of text (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

#define MAX_LINES 65536

static char *lines[MAX_LINES];
static int nlines;
static int reverse_flag;
static int numeric_flag;

/* Simple insertion sort (good enough for recovery tool) */
static void sort_lines(void) {
    for (int i = 1; i < nlines; i++) {
        int j = i - 1;
        char *saved = lines[i];
        while (j >= 0) {
            int cmp;
            if (numeric_flag) {
                long va = str_to_long(lines[j]);
                long vb = str_to_long(saved);
                cmp = (va > vb) - (va < vb);
                if (reverse_flag) cmp = -cmp;
            } else {
                cmp = str_cmp(lines[j], saved);
                if (reverse_flag) cmp = -cmp;
            }
            if (cmp <= 0) break;
            lines[j + 1] = lines[j];
            j--;
        }
        lines[j + 1] = saved;
    }
}

int sort_main(int argc, char **argv, char **envp) {
    (void)envp;
    reverse_flag = 0;
    numeric_flag = 0;
    int unique = 0;
    int first = 1;

    while (first < argc && argv[first][0] == '-') {
        for (int j = 1; argv[first][j]; j++) {
            if (argv[first][j] == 'r') reverse_flag = 1;
            else if (argv[first][j] == 'n') numeric_flag = 1;
            else if (argv[first][j] == 'u') unique = 1;
        }
        first++;
    }

    /* Read all input into buffer */
    static char buf[1048576]; /* 1MB buffer */
    ssize_t total = 0;

    if (first < argc) {
        for (int i = first; i < argc; i++) {
            int fd = sys_open(argv[i], O_RDONLY, 0);
            if (fd < 0) { eprint("sort: "); eprint(argv[i]); eprint(": cannot open\n"); continue; }
            ssize_t n;
            while ((n = sys_read(fd, buf + total, sizeof(buf) - (size_t)total - 1)) > 0) total += n;
            sys_close(fd);
        }
    } else {
        ssize_t n;
        while ((n = sys_read(0, buf + total, sizeof(buf) - (size_t)total - 1)) > 0) total += n;
    }
    buf[total] = '\0';

    /* Split into lines */
    nlines = 0;
    char *p = buf;
    while (*p && nlines < MAX_LINES) {
        lines[nlines++] = p;
        char *nl = str_chr(p, '\n');
        if (nl) { *nl = '\0'; p = nl + 1; }
        else break;
    }
    /* Remove empty trailing line */
    if (nlines > 0 && lines[nlines-1][0] == '\0') nlines--;

    sort_lines();

    /* Output */
    for (int i = 0; i < nlines; i++) {
        if (unique && i > 0 && str_cmp(lines[i], lines[i-1]) == 0) continue;
        print(lines[i]); print("\n");
    }
    return 0;
}

QEMT_ENTRY(sort_main)

/*
 * wc - word, line, byte count (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static void pad_num(unsigned long n, int w) {
    char buf[21];
    ulong_to_str(buf, n);
    int len = (int)str_len(buf);
    while (len++ < w) print(" ");
    print(buf);
}

static void count_stream(int fd, unsigned long *lines, unsigned long *words, unsigned long *bytes) {
    *lines = *words = *bytes = 0;
    char buf[4096];
    int in_word = 0;
    ssize_t n;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
        *bytes += (unsigned long)n;
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == '\n') (*lines)++;
            if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' || buf[i] == '\r') {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                (*words)++;
            }
        }
    }
}

int wc_main(int argc, char **argv, char **envp) {
    (void)envp;
    int show_l = 0, show_w = 0, show_c = 0;
    int first = 1;

    while (first < argc && argv[first][0] == '-' && argv[first][1] != '\0') {
        for (int j = 1; argv[first][j]; j++) {
            if (argv[first][j] == 'l') show_l = 1;
            else if (argv[first][j] == 'w') show_w = 1;
            else if (argv[first][j] == 'c') show_c = 1;
        }
        first++;
    }

    if (!show_l && !show_w && !show_c) { show_l = show_w = show_c = 1; }

    unsigned long tl = 0, tw = 0, tb = 0;
    int nfiles = argc - first;

    if (nfiles == 0) {
        unsigned long l, w, b;
        count_stream(0, &l, &w, &b);
        if (show_l) pad_num(l, 8);
        if (show_w) pad_num(w, 8);
        if (show_c) pad_num(b, 8);
        print("\n");
        return 0;
    }

    for (int i = first; i < argc; i++) {
        int fd = sys_open(argv[i], O_RDONLY, 0);
        if (fd < 0) { eprint("wc: "); eprint(argv[i]); eprint(": No such file\n"); continue; }
        unsigned long l, w, b;
        count_stream(fd, &l, &w, &b);
        sys_close(fd);

        if (show_l) pad_num(l, 8);
        if (show_w) pad_num(w, 8);
        if (show_c) pad_num(b, 8);
        print(" "); print(argv[i]); print("\n");

        tl += l; tw += w; tb += b;
    }

    if (nfiles > 1) {
        if (show_l) pad_num(tl, 8);
        if (show_w) pad_num(tw, 8);
        if (show_c) pad_num(tb, 8);
        print(" total\n");
    }
    return 0;
}

QEMT_ENTRY(wc_main)

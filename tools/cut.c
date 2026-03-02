/*
 * cut - extract columns from lines (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int parse_range(const char *s, int *ranges, int max_ranges) {
    int n = 0;
    while (*s && n < max_ranges - 1) {
        int start = 0, end = 0;
        while (*s >= '0' && *s <= '9') { start = start * 10 + (*s - '0'); s++; }
        if (*s == '-') {
            s++;
            if (*s >= '0' && *s <= '9') {
                while (*s >= '0' && *s <= '9') { end = end * 10 + (*s - '0'); s++; }
            } else {
                end = 99999;
            }
        } else {
            end = start;
        }
        if (start == 0) start = 1;
        ranges[n++] = start;
        ranges[n++] = end;
        if (*s == ',') s++;
    }
    return n / 2;
}

static int in_range(int pos, const int *ranges, int nranges) {
    for (int i = 0; i < nranges; i++)
        if (pos >= ranges[i * 2] && pos <= ranges[i * 2 + 1]) return 1;
    return 0;
}

static void cut_bytes(int fd, const int *ranges, int nranges) {
    char line[LINE_MAX];
    int len;
    while ((len = read_line(fd, line, LINE_MAX)) > 0) {
        for (int i = 0; i < len; i++)
            if (in_range(i + 1, ranges, nranges)) {
                char c = line[i];
                sys_write(1, &c, 1);
            }
        print("\n");
    }
}

static void cut_fields(int fd, char delim, const int *ranges, int nranges) {
    char line[LINE_MAX];
    int len;
    while ((len = read_line(fd, line, LINE_MAX)) > 0) {
        int field = 1, first = 1;
        char *start = line;
        for (char *p = line; ; p++) {
            if (*p == delim || *p == '\0') {
                char saved = *p;
                *p = '\0';
                if (in_range(field, ranges, nranges)) {
                    if (!first) { char d = delim; sys_write(1, &d, 1); }
                    print(start);
                    first = 0;
                }
                if (saved == '\0') break;
                *p = saved;
                start = p + 1;
                field++;
            }
        }
        print("\n");
    }
}

int cut_main(int argc, char **argv, char **envp) {
    (void)envp;
    char delim = '\t';
    int ranges[128];
    int nranges = 0;
    int mode = 0; /* 0=none, 1=bytes, 2=fields */
    const char *files[64];
    int nfiles = 0;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delim = argv[++i][0];
        } else if (str_cmp(argv[i], "-f") == 0 && i + 1 < argc) {
            mode = 2;
            nranges = parse_range(argv[++i], ranges, 128);
        } else if (str_cmp(argv[i], "-b") == 0 && i + 1 < argc) {
            mode = 1;
            nranges = parse_range(argv[++i], ranges, 128);
        } else if (str_cmp(argv[i], "-c") == 0 && i + 1 < argc) {
            mode = 1;
            nranges = parse_range(argv[++i], ranges, 128);
        } else if (starts_with(argv[i], "-d")) {
            delim = argv[i][2];
        } else if (starts_with(argv[i], "-f")) {
            mode = 2;
            nranges = parse_range(argv[i] + 2, ranges, 128);
        } else if (starts_with(argv[i], "-b") || starts_with(argv[i], "-c")) {
            mode = 1;
            nranges = parse_range(argv[i] + 2, ranges, 128);
        } else if (argv[i][0] != '-') {
            if (nfiles < 64) files[nfiles++] = argv[i];
        }
    }

    if (mode == 0 || nranges == 0) {
        print("Usage: cut -f FIELDS [-d DELIM] [file...]\n");
        print("       cut -b BYTES [file...]\n");
        print("       cut -c CHARS [file...]\n");
        return 1;
    }

    if (nfiles == 0) {
        if (mode == 1) cut_bytes(0, ranges, nranges);
        else cut_fields(0, delim, ranges, nranges);
    } else {
        for (int i = 0; i < nfiles; i++) {
            int fd = sys_open(files[i], O_RDONLY, 0);
            if (fd < 0) { eprint("cut: cannot open '"); eprint(files[i]); eprint("'\n"); continue; }
            if (mode == 1) cut_bytes(fd, ranges, nranges);
            else cut_fields(fd, delim, ranges, nranges);
            sys_close(fd);
        }
    }
    return 0;
}

QEMT_ENTRY(cut_main)

/*
 * nl - number lines (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int nl_main(int argc, char **argv, char **envp) {
    (void)envp;
    int width = 6;
    int skip_blank = 0;
    const char *file = NULL;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-ba") == 0) skip_blank = 0;
        else if (str_cmp(argv[i], "-bt") == 0) skip_blank = 1;
        else if (str_cmp(argv[i], "-w") == 0 && i + 1 < argc) width = (int)str_to_uint(argv[++i]);
        else if (str_cmp(argv[i], "-h") == 0 || str_cmp(argv[i], "--help") == 0) {
            print("Usage: nl [-ba|-bt] [-w WIDTH] [file]\n");
            print("  -ba  Number all lines\n  -bt  Number non-empty lines (default)\n");
            return 0;
        } else if (argv[i][0] != '-') file = argv[i];
    }

    int fd = 0;
    if (file) {
        fd = sys_open(file, O_RDONLY, 0);
        if (fd < 0) { eprint("nl: cannot open '"); eprint(file); eprint("'\n"); return 1; }
    }

    char buf[4096];
    char line[LINE_MAX];
    int lpos = 0;
    int lineno = 0;
    ssize_t n;

    while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                line[lpos] = '\0';
                if (lpos == 0 && skip_blank) {
                    print("\n");
                } else {
                    lineno++;
                    char num[12];
                    int nlen = int_to_str(num, lineno);
                    for (int j = nlen; j < width; j++) print(" ");
                    print(num);
                    print("\t");
                    print(line);
                    print("\n");
                }
                lpos = 0;
            } else if (lpos < LINE_MAX - 1) {
                line[lpos++] = buf[i];
            }
        }
    }
    /* Handle final line without trailing newline */
    if (lpos > 0) {
        line[lpos] = '\0';
        lineno++;
        char num[12];
        int nlen = int_to_str(num, lineno);
        for (int j = nlen; j < width; j++) print(" ");
        print(num);
        print("\t");
        print(line);
        print("\n");
    }

    if (file) sys_close(fd);
    return 0;
}

QEMT_ENTRY(nl_main)

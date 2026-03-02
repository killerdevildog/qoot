/*
 * uniq - filter adjacent duplicate lines (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int uniq_main(int argc, char **argv, char **envp) {
    (void)envp;
    int count_flag = 0, dup_only = 0;
    int first = 1;

    while (first < argc && argv[first][0] == '-') {
        for (int j = 1; argv[first][j]; j++) {
            if (argv[first][j] == 'c') count_flag = 1;
            else if (argv[first][j] == 'd') dup_only = 1;
        }
        first++;
    }

    int fd = 0;
    if (first < argc) {
        fd = sys_open(argv[first], O_RDONLY, 0);
        if (fd < 0) { eprint("uniq: cannot open '"); eprint(argv[first]); eprint("'\n"); return 1; }
    }

    char prev[4096] = "";
    int have_prev = 0;
    unsigned long count = 0;

    char buf[65536];
    ssize_t buf_len = 0;
    ssize_t line_start = 0;

    while (1) {
        ssize_t n = sys_read(fd, buf + buf_len, sizeof(buf) - (size_t)buf_len - 1);
        if (n <= 0 && buf_len == line_start) break;
        if (n > 0) buf_len += n;
        buf[buf_len] = '\0';

        while (line_start < buf_len) {
            ssize_t eol = line_start;
            while (eol < buf_len && buf[eol] != '\n') eol++;
            if (eol >= buf_len && n > 0) break;

            buf[eol] = '\0';
            char *line = buf + line_start;

            if (!have_prev || str_cmp(line, prev) != 0) {
                if (have_prev && (!dup_only || count > 1)) {
                    if (count_flag) {
                        char cbuf[21]; ulong_to_str(cbuf, count);
                        int len = (int)str_len(cbuf);
                        for (int k = 0; k < 7 - len; k++) print(" ");
                        print(cbuf); print(" ");
                    }
                    print(prev); print("\n");
                }
                str_ncpy(prev, line, sizeof(prev) - 1);
                have_prev = 1;
                count = 1;
            } else {
                count++;
            }

            line_start = eol + 1;
        }

        if (line_start > 0 && line_start < buf_len) {
            mem_move(buf, buf + line_start, (size_t)(buf_len - line_start));
            buf_len -= line_start;
        } else if (line_start >= buf_len) {
            buf_len = 0;
        }
        line_start = 0;
        if (n <= 0) break;
    }

    if (have_prev && (!dup_only || count > 1)) {
        if (count_flag) {
            char cbuf[21]; ulong_to_str(cbuf, count);
            int len = (int)str_len(cbuf);
            for (int k = 0; k < 7 - len; k++) print(" ");
            print(cbuf); print(" ");
        }
        print(prev); print("\n");
    }

    if (fd > 0) sys_close(fd);
    return 0;
}

QEMT_ENTRY(uniq_main)

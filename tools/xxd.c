/*
 * xxd - hex dump (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static const char hex_digits[] = "0123456789abcdef";

static void xxd_dump(int fd, int cols) {
    unsigned char buf[256];
    unsigned long offset = 0;
    ssize_t r;

    while ((r = sys_read(fd, buf, (size_t)cols)) > 0) {
        /* Offset */
        char off_str[16];
        for (int i = 7; i >= 0; i--) {
            off_str[i] = hex_digits[(offset >> ((7 - i) * 4)) & 0xf];
        }
        off_str[8] = ':';
        off_str[9] = ' ';
        sys_write(1, off_str, 10);

        /* Hex */
        for (int i = 0; i < cols; i++) {
            if (i < (int)r) {
                char h[3] = {hex_digits[buf[i] >> 4], hex_digits[buf[i] & 0xf], ' '};
                sys_write(1, h, 3);
            } else {
                sys_write(1, "   ", 3);
            }
            if (i == cols / 2 - 1) sys_write(1, " ", 1);
        }

        /* ASCII */
        sys_write(1, " ", 1);
        for (int i = 0; i < (int)r; i++) {
            char c = (buf[i] >= 32 && buf[i] <= 126) ? (char)buf[i] : '.';
            sys_write(1, &c, 1);
        }
        sys_write(1, "\n", 1);

        offset += (unsigned long)r;
    }
}

static void xxd_reverse(int fd, int out_fd) {
    char line[512];
    int len;
    while ((len = read_line(fd, line, sizeof(line))) > 0) {
        /* Skip offset and colon */
        char *p = line;
        while (*p && *p != ':') p++;
        if (*p == ':') p++;
        while (*p == ' ') p++;

        /* Parse hex bytes */
        while (*p) {
            if (*p == ' ') { p++; continue; }
            /* Stop at ASCII section */
            if (*p == ' ' && *(p+1) == ' ') break;
            unsigned char byte = 0;
            for (int i = 0; i < 2 && *p; i++, p++) {
                byte <<= 4;
                if (*p >= '0' && *p <= '9') byte |= (unsigned char)(*p - '0');
                else if (*p >= 'a' && *p <= 'f') byte |= (unsigned char)(*p - 'a' + 10);
                else if (*p >= 'A' && *p <= 'F') byte |= (unsigned char)(*p - 'A' + 10);
                else break;
            }
            sys_write(out_fd, &byte, 1);
        }
    }
}

int xxd_main(int argc, char **argv, char **envp) {
    (void)envp;
    int cols = 16, reverse = 0;
    const char *file = NULL;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-r") == 0) reverse = 1;
        else if (str_cmp(argv[i], "-c") == 0 && i + 1 < argc)
            cols = (int)str_to_uint(argv[++i]);
        else if (starts_with(argv[i], "-c"))
            cols = (int)str_to_uint(argv[i] + 2);
        else if (str_cmp(argv[i], "-h") == 0 || str_cmp(argv[i], "--help") == 0) {
            print("Usage: xxd [-r] [-c COLS] [file]\n");
            print("  -r       Reverse: hex to binary\n");
            print("  -c COLS  Bytes per line (default: 16)\n");
            return 0;
        } else if (argv[i][0] != '-') {
            file = argv[i];
        }
    }

    if (cols < 1) cols = 16;
    if (cols > 256) cols = 256;

    int fd = 0;
    if (file) {
        fd = sys_open(file, O_RDONLY, 0);
        if (fd < 0) { eprint("xxd: cannot open '"); eprint(file); eprint("'\n"); return 1; }
    }

    if (reverse) xxd_reverse(fd, 1);
    else xxd_dump(fd, cols);

    if (file) sys_close(fd);
    return 0;
}

QEMT_ENTRY(xxd_main)

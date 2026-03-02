/*
 * mktemp - create temporary file or directory (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int mktemp_main(int argc, char **argv, char **envp) {
    (void)envp;
    int make_dir = 0;
    const char *tmpdir = "/tmp";
    const char *tmpl = NULL;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-d") == 0) make_dir = 1;
        else if (str_cmp(argv[i], "-p") == 0 && i + 1 < argc) tmpdir = argv[++i];
        else if (str_cmp(argv[i], "-h") == 0 || str_cmp(argv[i], "--help") == 0) {
            print("Usage: mktemp [-d] [-p DIR] [TEMPLATE]\n");
            print("  -d       Create directory\n");
            print("  -p DIR   Use DIR instead of /tmp\n");
            print("  TEMPLATE Should contain XXXXXX\n");
            return 0;
        } else if (argv[i][0] != '-') {
            tmpl = argv[i];
        }
    }

    /* Build path */
    char path[PATH_MAX];
    if (tmpl) {
        if (tmpl[0] == '/') {
            str_cpy(path, tmpl);
        } else {
            str_cpy(path, tmpdir);
            str_cat(path, "/");
            str_cat(path, tmpl);
        }
    } else {
        str_cpy(path, tmpdir);
        str_cat(path, "/tmp.XXXXXX");
    }

    /* Find and replace XXXXXX */
    char *xx = str_str(path, "XXXXXX");
    if (!xx) {
        eprint("mktemp: template must contain XXXXXX\n");
        return 1;
    }

    /* Generate random suffix */
    unsigned char randbuf[6];
    ssize_t r = sys_getrandom(randbuf, 6, 0);
    if (r < 6) {
        /* Fallback: use pid + time */
        unsigned int seed = (unsigned int)sys_getpid() ^
                           (unsigned int)sys_getuid();
        struct timeval tv;
        sys_gettimeofday(&tv, NULL);
        seed ^= (unsigned int)tv.tv_usec;
        for (int i = 0; i < 6; i++) {
            seed = seed * 1103515245 + 12345;
            randbuf[i] = (unsigned char)(seed >> 16);
        }
    }

    const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 6; i++)
        xx[i] = chars[randbuf[i] % 62];

    if (make_dir) {
        if (sys_mkdir(path, 0700) < 0) {
            eprint("mktemp: cannot create directory\n");
            return 1;
        }
    } else {
        int fd = sys_open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd < 0) {
            eprint("mktemp: cannot create file\n");
            return 1;
        }
        sys_close(fd);
    }

    print(path);
    print("\n");
    return 0;
}

QEMT_ENTRY(mktemp_main)

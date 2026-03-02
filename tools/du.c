/*
 * du - disk usage summary (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static void pad_left(const char *s, int w) {
    int l = (int)str_len(s);
    while (l++ < w) print(" ");
    print(s);
}

static unsigned long du_path(const char *path, int summary, int human) {
    struct stat st;
    if (sys_lstat(path, &st) < 0) return 0;

    if (!S_ISDIR(st.st_mode)) {
        unsigned long kb = (unsigned long)((st.st_blocks * 512 + 1023) / 1024);
        if (!summary) {
            char buf[21];
            if (human) {
                unsigned long v = kb;
                const char *suf = "K";
                if (v >= 1024) { v /= 1024; suf = "M"; }
                if (v >= 1024) { v /= 1024; suf = "G"; }
                ulong_to_str(buf, v); str_cat(buf, suf);
            } else {
                ulong_to_str(buf, kb);
            }
            pad_left(buf, 8); print("\t"); print(path); print("\n");
        }
        return kb;
    }

    unsigned long total = (unsigned long)((st.st_blocks * 512 + 1023) / 1024);

    int fd = sys_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) return total;

    char dbuf[4096];
    int nread;
    while ((nread = sys_getdents64(fd, dbuf, sizeof(dbuf))) > 0) {
        int pos = 0;
        while (pos < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(dbuf + pos);
            if (str_cmp(d->d_name, ".") != 0 && str_cmp(d->d_name, "..") != 0) {
                char child[PATH_MAX];
                str_cpy(child, path);
                str_cat(child, "/");
                str_cat(child, d->d_name);
                total += du_path(child, summary, human);
            }
            pos += d->d_reclen;
        }
    }
    sys_close(fd);

    if (!summary) {
        char buf[21];
        if (human) {
            unsigned long v = total;
            const char *suf = "K";
            if (v >= 1024) { v /= 1024; suf = "M"; }
            if (v >= 1024) { v /= 1024; suf = "G"; }
            ulong_to_str(buf, v); str_cat(buf, suf);
        } else {
            ulong_to_str(buf, total);
        }
        pad_left(buf, 8); print("\t"); print(path); print("\n");
    }

    return total;
}

int du_main(int argc, char **argv, char **envp) {
    (void)envp;
    int summary = 0, human = 0;
    int first = 1;

    while (first < argc && argv[first][0] == '-') {
        for (int j = 1; argv[first][j]; j++) {
            if (argv[first][j] == 's') summary = 1;
            else if (argv[first][j] == 'h') human = 1;
        }
        first++;
    }

    if (first >= argc) {
        unsigned long total = du_path(".", summary, human);
        if (summary) {
            char buf[21];
            if (human) {
                unsigned long v = total;
                const char *suf = "K";
                if (v >= 1024) { v /= 1024; suf = "M"; }
                if (v >= 1024) { v /= 1024; suf = "G"; }
                ulong_to_str(buf, v); str_cat(buf, suf);
            } else {
                ulong_to_str(buf, total);
            }
            pad_left(buf, 8); print("\t.\n");
        }
        return 0;
    }

    for (int i = first; i < argc; i++) {
        unsigned long total = du_path(argv[i], summary, human);
        if (summary) {
            char buf[21];
            if (human) {
                unsigned long v = total;
                const char *suf = "K";
                if (v >= 1024) { v /= 1024; suf = "M"; }
                if (v >= 1024) { v /= 1024; suf = "G"; }
                ulong_to_str(buf, v); str_cat(buf, suf);
            } else {
                ulong_to_str(buf, total);
            }
            pad_left(buf, 8); print("\t"); print(argv[i]); print("\n");
        }
    }
    return 0;
}

QEMT_ENTRY(du_main)

/*
 * find - search for files in directory hierarchy (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int match_name(const char *name, const char *pattern) {
    /* Simple glob matching with * and ? */
    while (*pattern) {
        if (*pattern == '*') {
            pattern++;
            if (*pattern == '\0') return 1;
            while (*name) {
                if (match_name(name, pattern)) return 1;
                name++;
            }
            return 0;
        } else if (*pattern == '?') {
            if (*name == '\0') return 0;
            name++; pattern++;
        } else {
            if (*name != *pattern) return 0;
            name++; pattern++;
        }
    }
    return (*name == '\0');
}

static void find_recurse(const char *path, const char *name_pattern, const char *type_filter) {
    int fd = sys_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) {
        /* It's a file, check it */
        struct stat st;
        if (sys_lstat(path, &st) == 0) {
            int match = 1;
            if (name_pattern) {
                const char *base = str_rchr(path, '/');
                base = base ? base + 1 : path;
                if (!match_name(base, name_pattern)) match = 0;
            }
            if (type_filter) {
                if (str_cmp(type_filter, "f") == 0 && !S_ISREG(st.st_mode)) match = 0;
                if (str_cmp(type_filter, "d") == 0 && !S_ISDIR(st.st_mode)) match = 0;
                if (str_cmp(type_filter, "l") == 0 && !S_ISLNK(st.st_mode)) match = 0;
            }
            if (match) { print(path); print("\n"); }
        }
        return;
    }

    /* Check the directory itself */
    if (!name_pattern || match_name(str_rchr(path, '/') ? str_rchr(path, '/') + 1 : path, name_pattern)) {
        if (!type_filter || str_cmp(type_filter, "d") == 0) {
            print(path); print("\n");
        }
    }

    char buf[4096];
    int nread;
    while ((nread = sys_getdents64(fd, buf, sizeof(buf))) > 0) {
        int pos = 0;
        while (pos < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);
            if (str_cmp(d->d_name, ".") != 0 && str_cmp(d->d_name, "..") != 0) {
                char child[PATH_MAX];
                str_cpy(child, path);
                if (child[str_len(child) - 1] != '/') str_cat(child, "/");
                str_cat(child, d->d_name);

                struct stat st;
                if (sys_lstat(child, &st) == 0) {
                    if (S_ISDIR(st.st_mode)) {
                        find_recurse(child, name_pattern, type_filter);
                    } else {
                        int match = 1;
                        if (name_pattern && !match_name(d->d_name, name_pattern)) match = 0;
                        if (type_filter) {
                            if (str_cmp(type_filter, "f") == 0 && !S_ISREG(st.st_mode)) match = 0;
                            if (str_cmp(type_filter, "d") == 0) match = 0;
                            if (str_cmp(type_filter, "l") == 0 && !S_ISLNK(st.st_mode)) match = 0;
                        }
                        if (match) { print(child); print("\n"); }
                    }
                }
            }
            pos += d->d_reclen;
        }
    }
    sys_close(fd);
}

int find_main(int argc, char **argv, char **envp) {
    (void)envp;
    const char *start = ".";
    const char *name_pattern = NULL;
    const char *type_filter = NULL;

    if (argc > 1 && str_cmp(argv[1], "--help") == 0) {
        print("Usage: find [path] [-name pattern] [-type f|d|l]\n");
        return 0;
    }

    int i = 1;
    if (i < argc && argv[i][0] != '-') { start = argv[i++]; }

    while (i < argc) {
        if (str_cmp(argv[i], "-name") == 0 && i + 1 < argc) {
            name_pattern = argv[++i]; i++;
        } else if (str_cmp(argv[i], "-type") == 0 && i + 1 < argc) {
            type_filter = argv[++i]; i++;
        } else { i++; }
    }

    find_recurse(start, name_pattern, type_filter);
    return 0;
}

QEMT_ENTRY(find_main)

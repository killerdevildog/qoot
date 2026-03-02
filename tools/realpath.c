/*
 * realpath - resolve absolute file path (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

/* Resolve symlinks and canonicalize path using /proc */
static int resolve_path(const char *path, char *out) {
    /* First try readlink on /proc/self/fd approach:
       open the path, then readlink the fd */
    int fd = sys_open(path, O_RDONLY, 0);
    if (fd < 0) {
        /* Try as directory */
        fd = sys_open(path, O_RDONLY | 0200000 /* O_DIRECTORY */, 0);
    }
    if (fd >= 0) {
        char proc_path[64];
        str_cpy(proc_path, "/proc/self/fd/");
        char num[16];
        int_to_str(num, fd);
        str_cat(proc_path, num);

        ssize_t n = sys_readlink(proc_path, out, PATH_MAX - 1);
        sys_close(fd);
        if (n > 0) {
            out[n] = '\0';
            return 0;
        }
    }

    /* Fallback: manual canonicalization */
    char tmp[PATH_MAX];
    if (path[0] != '/') {
        sys_getcwd(tmp, PATH_MAX);
        str_cat(tmp, "/");
        str_cat(tmp, path);
    } else {
        str_cpy(tmp, path);
    }

    /* Normalize . and .. */
    char *components[256];
    int ncomp = 0;
    char *p = tmp;

    while (*p) {
        while (*p == '/') p++;
        if (*p == '\0') break;
        char *start = p;
        while (*p && *p != '/') p++;
        size_t len = p - start;

        if (len == 1 && start[0] == '.') {
            continue;
        } else if (len == 2 && start[0] == '.' && start[1] == '.') {
            if (ncomp > 0) ncomp--;
        } else {
            if (*p) *p++ = '\0';
            components[ncomp++] = start;
        }
    }

    out[0] = '/';
    out[1] = '\0';
    for (int i = 0; i < ncomp; i++) {
        if (i > 0) str_cat(out, "/");
        else out[0] = '\0';  /* Remove initial / for first component */
        if (i == 0) { out[0] = '/'; out[1] = '\0'; }
        str_cat(out, components[i]);
    }
    if (out[0] == '\0') { out[0] = '/'; out[1] = '\0'; }

    return 0;
}

int realpath_main(int argc, char **argv, char **envp) {
    (void)envp;
    int quiet = 0;
    int first_arg = 1;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-q") == 0 || str_cmp(argv[i], "--quiet") == 0) {
            quiet = 1;
            first_arg = i + 1;
        } else if (str_cmp(argv[i], "--help") == 0 || str_cmp(argv[i], "-h") == 0) {
            print("Usage: realpath [-q] PATH...\n");
            print("Resolve each PATH to an absolute path.\n  -q  Suppress errors\n");
            return 0;
        } else {
            break;
        }
    }

    if (first_arg >= argc) {
        eprint("realpath: missing operand\n");
        return 1;
    }

    int ret = 0;
    char resolved[PATH_MAX];

    for (int i = first_arg; i < argc; i++) {
        if (resolve_path(argv[i], resolved) == 0) {
            print(resolved);
            print("\n");
        } else {
            if (!quiet) {
                eprint("realpath: "); eprint(argv[i]); eprint(": cannot resolve\n");
            }
            ret = 1;
        }
    }

    return ret;
}

QEMT_ENTRY(realpath_main)

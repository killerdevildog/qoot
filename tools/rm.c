/*
 * rm - remove files and directories (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int rm_recursive(const char *path);

static int rm_entry(const char *path, int recursive, int force) {
    struct stat st;
    if (sys_lstat(path, &st) < 0) {
        if (!force) { eprint("rm: cannot remove '"); eprint(path); eprint("': No such file\n"); }
        return force ? 0 : 1;
    }

    if (S_ISDIR(st.st_mode)) {
        if (!recursive) {
            eprint("rm: cannot remove '"); eprint(path); eprint("': Is a directory\n");
            return 1;
        }
        return rm_recursive(path);
    }

    if (sys_unlink(path) < 0) {
        eprint("rm: cannot remove '"); eprint(path); eprint("'\n");
        return 1;
    }
    return 0;
}

static int rm_recursive(const char *path) {
    int fd = sys_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) { eprint("rm: cannot open directory '"); eprint(path); eprint("'\n"); return 1; }

    char buf[4096];
    int nread, ret = 0;
    while ((nread = sys_getdents64(fd, buf, sizeof(buf))) > 0) {
        int pos = 0;
        while (pos < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);
            if (str_cmp(d->d_name, ".") != 0 && str_cmp(d->d_name, "..") != 0) {
                char child[PATH_MAX];
                str_cpy(child, path);
                str_cat(child, "/");
                str_cat(child, d->d_name);
                ret |= rm_entry(child, 1, 0);
            }
            pos += d->d_reclen;
        }
    }
    sys_close(fd);

    if (sys_rmdir(path) < 0) {
        eprint("rm: cannot remove directory '"); eprint(path); eprint("'\n");
        ret = 1;
    }
    return ret;
}

int rm_main(int argc, char **argv, char **envp) {
    (void)envp;
    int recursive = 0, force = 0;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: rm [-rf] file...\n");
        print("  -r  Remove directories recursively\n  -f  Force (ignore nonexistent)\n");
        return 0;
    }

    while (first < argc && argv[first][0] == '-' && argv[first][1] != '\0') {
        for (int j = 1; argv[first][j]; j++) {
            if (argv[first][j] == 'r' || argv[first][j] == 'R') recursive = 1;
            else if (argv[first][j] == 'f') force = 1;
            else { eprint("rm: unknown option\n"); return 1; }
        }
        first++;
    }

    if (first >= argc) {
        if (!force) eprint("rm: missing operand\n");
        return force ? 0 : 1;
    }

    int ret = 0;
    for (int i = first; i < argc; i++)
        ret |= rm_entry(argv[i], recursive, force);
    return ret;
}

QEMT_ENTRY(rm_main)

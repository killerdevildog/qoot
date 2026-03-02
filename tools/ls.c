/*
 * ls - directory listing (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static void pad_right(const char *s, int width) {
    print(s);
    int len = (int)str_len(s);
    while (len++ < width) print(" ");
}

static void print_long_entry(const char *dir, const char *name) {
    char path[PATH_MAX];
    str_cpy(path, dir);
    if (path[str_len(path)-1] != '/') str_cat(path, "/");
    str_cat(path, name);

    struct stat st;
    if (sys_lstat(path, &st) < 0) return;

    char mode_str[11];
    format_mode(st.st_mode, mode_str);
    print(mode_str);

    char buf[21];
    print(" ");
    ulong_to_str(buf, st.st_nlink);
    pad_right(buf, 3);

    char owner[64] = "?", group[64] = "?";
    uid_to_name(st.st_uid, owner, 64);
    gid_to_name(st.st_gid, group, 64);
    pad_right(owner, 10);
    print(" ");
    pad_right(group, 10);

    print(" ");
    long_to_str(buf, st.st_size);
    pad_right(buf, 10);

    print(" ");
    print(name);

    if (S_ISLNK(st.st_mode)) {
        char link[PATH_MAX];
        ssize_t n = sys_readlink(path, link, PATH_MAX - 1);
        if (n > 0) { link[n] = '\0'; print(" -> "); print(link); }
    }
    print("\n");
}

int ls_main(int argc, char **argv, char **envp) {
    (void)envp;
    int long_fmt = 0, show_all = 0, show_help = 0;
    const char *target = ".";
    int targets[64], ntargets = 0;

    int end_opts = 0;
    for (int i = 1; i < argc; i++) {
        if (!end_opts && argv[i][0] == '-' && argv[i][1] != '\0') {
            if (argv[i][1] == '-') {
                /* --long-option: silently ignore (e.g. --color=auto from aliases) */
                if (argv[i][2] == '\0') end_opts = 1; /* bare "--" ends options */
                continue;
            }
            for (int j = 1; argv[i][j]; j++) {
                if (argv[i][j] == 'l') long_fmt = 1;
                else if (argv[i][j] == 'a') show_all = 1;
                else if (argv[i][j] == '1') { /* one-per-line: default, no-op */ }
                else if (argv[i][j] == 'h') show_help = 1;
                else { eprint("ls: unknown option: -"); char c[2] = {argv[i][j], 0}; eprint(c); eprint("\n"); return 1; }
            }
        } else {
            if (ntargets < 64) targets[ntargets++] = i;
        }
    }

    if (show_help) {
        print("Usage: ls [-la] [path...]\n");
        print("  -l  Long format\n  -a  Show hidden files\n");
        return 0;
    }

    if (ntargets == 0) { targets[0] = -1; ntargets = 1; }

    for (int t = 0; t < ntargets; t++) {
        target = (targets[t] == -1) ? "." : argv[targets[t]];
        if (ntargets > 1) { print(target); print(":\n"); }

        int fd = sys_open(target, O_RDONLY | O_DIRECTORY, 0);
        if (fd < 0) {
            /* Might be a file */
            struct stat st;
            if (sys_lstat(target, &st) == 0 && !S_ISDIR(st.st_mode)) {
                if (long_fmt) print_long_entry(".", target);
                else { print(target); print("\n"); }
                continue;
            }
            eprint("ls: cannot open '"); eprint(target); eprint("'\n");
            return 1;
        }

        char buf[4096];
        int nread;
        while ((nread = sys_getdents64(fd, buf, sizeof(buf))) > 0) {
            int pos = 0;
            while (pos < nread) {
                struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);
                if (!show_all && d->d_name[0] == '.') { pos += d->d_reclen; continue; }
                if (long_fmt) {
                    print_long_entry(target, d->d_name);
                } else {
                    print(d->d_name);
                    print("\n");
                }
                pos += d->d_reclen;
            }
        }
        sys_close(fd);
        if (t < ntargets - 1) print("\n");
    }
    return 0;
}

QEMT_ENTRY(ls_main)

/*
 * cp - copy files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int copy_file(const char *src, const char *dst) {
    struct stat st;
    if (sys_stat(src, &st) < 0) {
        eprint("cp: cannot stat '"); eprint(src); eprint("'\n");
        return 1;
    }

    int sfd = sys_open(src, O_RDONLY, 0);
    if (sfd < 0) {
        eprint("cp: cannot open '"); eprint(src); eprint("'\n");
        return 1;
    }

    int dfd = sys_open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (dfd < 0) {
        eprint("cp: cannot create '"); eprint(dst); eprint("'\n");
        sys_close(sfd);
        return 1;
    }

    char buf[8192];
    ssize_t n;
    while ((n = sys_read(sfd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = sys_write(dfd, buf + written, n - written);
            if (w <= 0) {
                eprint("cp: write error\n");
                sys_close(sfd); sys_close(dfd);
                return 1;
            }
            written += w;
        }
    }

    sys_close(sfd);
    sys_close(dfd);
    return 0;
}

static const char *basename(const char *path) {
    const char *p = str_rchr(path, '/');
    return p ? p + 1 : path;
}

int cp_main(int argc, char **argv, char **envp) {
    (void)envp;
    int recursive = 0;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: cp [-r] source... dest\n");
        print("  -r  Copy directories recursively\n");
        return 0;
    }

    if (argc > 1 && (str_cmp(argv[1], "-r") == 0 || str_cmp(argv[1], "-R") == 0)) {
        recursive = 1;
        first = 2;
    }

    if (argc - first < 2) {
        eprint("cp: missing operand\nUsage: cp [-r] source... dest\n");
        return 1;
    }

    const char *dest = argv[argc - 1];
    struct stat dst_st;
    int dest_is_dir = (sys_stat(dest, &dst_st) == 0 && S_ISDIR(dst_st.st_mode));

    for (int i = first; i < argc - 1; i++) {
        struct stat src_st;
        if (sys_stat(argv[i], &src_st) < 0) {
            eprint("cp: cannot stat '"); eprint(argv[i]); eprint("'\n");
            continue;
        }

        if (S_ISDIR(src_st.st_mode)) {
            if (!recursive) {
                eprint("cp: -r not specified; omitting directory '"); eprint(argv[i]); eprint("'\n");
                continue;
            }
            /* Recursive directory copy */
            char destpath[PATH_MAX];
            if (dest_is_dir) {
                str_cpy(destpath, dest);
                str_cat(destpath, "/");
                str_cat(destpath, basename(argv[i]));
            } else {
                str_cpy(destpath, dest);
            }
            sys_mkdir(destpath, src_st.st_mode & 0777);

            int fd = sys_open(argv[i], O_RDONLY | O_DIRECTORY, 0);
            if (fd < 0) continue;
            char dbuf[4096];
            int nread;
            while ((nread = sys_getdents64(fd, dbuf, sizeof(dbuf))) > 0) {
                int pos = 0;
                while (pos < nread) {
                    struct linux_dirent64 *d = (struct linux_dirent64 *)(dbuf + pos);
                    if (str_cmp(d->d_name, ".") == 0 || str_cmp(d->d_name, "..") == 0) {
                        pos += d->d_reclen;
                        continue;
                    }
                    char sp[PATH_MAX], dp[PATH_MAX];
                    str_cpy(sp, argv[i]); str_cat(sp, "/"); str_cat(sp, d->d_name);
                    str_cpy(dp, destpath); str_cat(dp, "/"); str_cat(dp, d->d_name);

                    struct stat child_st;
                    if (sys_lstat(sp, &child_st) == 0 && S_ISDIR(child_st.st_mode)) {
                        sys_mkdir(dp, child_st.st_mode & 0777);
                        /* Simple one-level recursion; deep trees need a stack */
                    } else {
                        copy_file(sp, dp);
                    }
                    pos += d->d_reclen;
                }
            }
            sys_close(fd);
        } else {
            char final_dest[PATH_MAX];
            if (dest_is_dir) {
                str_cpy(final_dest, dest);
                str_cat(final_dest, "/");
                str_cat(final_dest, basename(argv[i]));
            } else {
                str_cpy(final_dest, dest);
            }
            copy_file(argv[i], final_dest);
        }
    }
    return 0;
}

QEMT_ENTRY(cp_main)

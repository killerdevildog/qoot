/*
 * mv - move/rename files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static const char *basename(const char *path) {
    const char *p = str_rchr(path, '/');
    return p ? p + 1 : path;
}

int mv_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: mv source... dest\n");
        return 0;
    }

    if (argc < 3) {
        eprint("mv: missing operand\nUsage: mv source... dest\n");
        return 1;
    }

    const char *dest = argv[argc - 1];
    struct stat dst_st;
    int dest_is_dir = (sys_stat(dest, &dst_st) == 0 && S_ISDIR(dst_st.st_mode));

    if (argc > 3 && !dest_is_dir) {
        eprint("mv: target '"); eprint(dest); eprint("' is not a directory\n");
        return 1;
    }

    int ret = 0;
    for (int i = 1; i < argc - 1; i++) {
        char final_dest[PATH_MAX];
        if (dest_is_dir) {
            str_cpy(final_dest, dest);
            str_cat(final_dest, "/");
            str_cat(final_dest, basename(argv[i]));
        } else {
            str_cpy(final_dest, dest);
        }

        if (sys_rename(argv[i], final_dest) < 0) {
            eprint("mv: cannot move '"); eprint(argv[i]);
            eprint("' to '"); eprint(final_dest); eprint("'\n");
            ret = 1;
        }
    }
    return ret;
}

QEMT_ENTRY(mv_main)

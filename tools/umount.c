/*
 * umount - unmount filesystems (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

#define MNT_FORCE  1
#define MNT_DETACH 2

int umount_main(int argc, char **argv, char **envp) {
    (void)envp;
    int flags = 0;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: umount [-fl] mountpoint...\n");
        print("  -f  Force unmount\n  -l  Lazy unmount (detach)\n");
        return 0;
    }

    while (first < argc && argv[first][0] == '-') {
        for (int j = 1; argv[first][j]; j++) {
            if (argv[first][j] == 'f') flags |= MNT_FORCE;
            else if (argv[first][j] == 'l') flags |= MNT_DETACH;
            else { eprint("umount: unknown option\n"); return 1; }
        }
        first++;
    }

    if (first >= argc) {
        eprint("umount: missing mountpoint\n");
        return 1;
    }

    int ret = 0;
    for (int i = first; i < argc; i++) {
        if (sys_umount2(argv[i], flags) < 0) {
            eprint("umount: "); eprint(argv[i]); eprint(": unmount failed\n");
            ret = 1;
        }
    }
    return ret;
}

QEMT_ENTRY(umount_main)

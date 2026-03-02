/*
 * touch - create file or update timestamps (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int touch_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: touch file...\n");
        return 0;
    }

    if (argc < 2) {
        eprint("touch: missing operand\n");
        return 1;
    }

    int ret = 0;
    for (int i = 1; i < argc; i++) {
        /* Try to update timestamps first */
        struct timespec times[2];
        times[0].tv_sec = 0; times[0].tv_nsec = UTIME_NOW;
        times[1].tv_sec = 0; times[1].tv_nsec = UTIME_NOW;

        if (sys_utimensat(AT_FDCWD, argv[i], times, 0) < 0) {
            /* File doesn't exist, create it */
            int fd = sys_open(argv[i], O_WRONLY | O_CREAT, 0644);
            if (fd < 0) {
                eprint("touch: cannot touch '"); eprint(argv[i]); eprint("'\n");
                ret = 1;
                continue;
            }
            sys_close(fd);
        }
    }
    return ret;
}

QEMT_ENTRY(touch_main)

/*
 * sleep - delay for a specified time (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

#define SYS_nanosleep 35

static long sys_nanosleep(const struct timespec *req, struct timespec *rem) {
    return syscall2(SYS_nanosleep, (long)req, (long)rem);
}

int sleep_main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc < 2) { eprint("Usage: sleep seconds\n"); return 1; }

    unsigned long secs = str_to_uint(argv[1]);
    struct timespec ts;
    ts.tv_sec = (long)secs;
    ts.tv_nsec = 0;
    sys_nanosleep(&ts, NULL);
    return 0;
}

QEMT_ENTRY(sleep_main)

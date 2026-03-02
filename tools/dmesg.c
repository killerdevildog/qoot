/*
 * dmesg - print kernel ring buffer (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

/* syslog actions */
#define SYSLOG_ACTION_READ_ALL    3
#define SYSLOG_ACTION_SIZE_BUFFER 10

int dmesg_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: dmesg\n");
        print("Prints the kernel ring buffer (requires root).\n");
        return 0;
    }

    /* Try /dev/kmsg first (non-root friendly on some systems) */
    int fd = sys_open("/dev/kmsg", O_RDONLY | 0x800 /* O_NONBLOCK */, 0);
    if (fd >= 0) {
        /* /dev/kmsg gives structured output, harder to parse */
        sys_close(fd);
    }

    /* Use syslog syscall */
    int size = sys_syslog(SYSLOG_ACTION_SIZE_BUFFER, NULL, 0);
    if (size <= 0) {
        /* Fallback: try reading /var/log/dmesg or /var/log/kern.log */
        fd = sys_open("/var/log/kern.log", O_RDONLY, 0);
        if (fd < 0) fd = sys_open("/var/log/dmesg", O_RDONLY, 0);
        if (fd < 0) {
            eprint("dmesg: permission denied (try running as root)\n");
            return 1;
        }
        char buf[4096];
        ssize_t n;
        while ((n = sys_read(fd, buf, sizeof(buf))) > 0)
            sys_write(1, buf, n);
        sys_close(fd);
        return 0;
    }

    /* Allocate buffer on stack (limit to 256K) */
    if (size > 262144) size = 262144;
    char buf[262144];
    int n = sys_syslog(SYSLOG_ACTION_READ_ALL, buf, size);
    if (n > 0) {
        sys_write(1, buf, n);
        if (buf[n-1] != '\n') print("\n");
    }
    return 0;
}

QEMT_ENTRY(dmesg_main)

/*
 * uptime - show system uptime (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int uptime_main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;

    int fd = sys_open("/proc/uptime", O_RDONLY, 0);
    if (fd < 0) { eprint("uptime: cannot read /proc/uptime\n"); return 1; }

    char buf[256];
    ssize_t n = sys_read(fd, buf, sizeof(buf) - 1);
    sys_close(fd);
    if (n <= 0) return 1;
    buf[n] = '\0';

    /* Parse uptime seconds (first number before the dot) */
    unsigned long secs = str_to_uint(buf);

    unsigned long days = secs / 86400;
    unsigned long hours = (secs % 86400) / 3600;
    unsigned long mins = (secs % 3600) / 60;

    char nbuf[21];
    print("up ");
    if (days > 0) {
        ulong_to_str(nbuf, days); print(nbuf);
        print(days == 1 ? " day, " : " days, ");
    }
    ulong_to_str(nbuf, hours); print(nbuf); print(":");
    if (mins < 10) print("0");
    ulong_to_str(nbuf, mins); print(nbuf);

    /* Load average */
    fd = sys_open("/proc/loadavg", O_RDONLY, 0);
    if (fd >= 0) {
        n = sys_read(fd, buf, sizeof(buf) - 1);
        sys_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            /* First 3 space-separated values */
            print(",  load average: ");
            int fields = 0;
            char *p = buf;
            while (*p && fields < 3) {
                if (fields > 0) print(", ");
                char *sp = str_chr(p, ' ');
                if (sp) *sp = '\0';
                print(p);
                fields++;
                if (sp) p = sp + 1; else break;
            }
        }
    }
    print("\n");
    return 0;
}

QEMT_ENTRY(uptime_main)

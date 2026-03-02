/*
 * mount - mount filesystems (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static unsigned long parse_flags(const char *opts) {
    unsigned long flags = 0;
    char tmp[1024];
    str_ncpy(tmp, opts, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *p = tmp;
    while (*p) {
        char *comma = str_chr(p, ',');
        if (comma) *comma = '\0';

        if (str_cmp(p, "ro") == 0) flags |= MS_RDONLY;
        else if (str_cmp(p, "nosuid") == 0) flags |= MS_NOSUID;
        else if (str_cmp(p, "nodev") == 0) flags |= MS_NODEV;
        else if (str_cmp(p, "noexec") == 0) flags |= MS_NOEXEC;
        else if (str_cmp(p, "remount") == 0) flags |= MS_REMOUNT;
        else if (str_cmp(p, "bind") == 0) flags |= MS_BIND;

        if (comma) p = comma + 1;
        else break;
    }
    return flags;
}

int mount_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: mount [-t type] [-o options] device mountpoint\n");
        print("       mount              (show mounted filesystems)\n");
        return 0;
    }

    /* No args: show mount table */
    if (argc == 1) {
        int fd = sys_open("/proc/mounts", O_RDONLY, 0);
        if (fd < 0) { eprint("mount: cannot read /proc/mounts\n"); return 1; }
        char buf[8192];
        ssize_t n;
        while ((n = sys_read(fd, buf, sizeof(buf))) > 0)
            sys_write(1, buf, n);
        sys_close(fd);
        return 0;
    }

    const char *fstype = NULL;
    const char *options = NULL;
    int i = 1;

    while (i < argc - 2) {
        if (str_cmp(argv[i], "-t") == 0 && i + 1 < argc) {
            fstype = argv[i + 1]; i += 2;
        } else if (str_cmp(argv[i], "-o") == 0 && i + 1 < argc) {
            options = argv[i + 1]; i += 2;
        } else break;
    }

    if (argc - i < 2) {
        eprint("mount: missing device or mountpoint\n");
        return 1;
    }

    const char *device = argv[i];
    const char *target = argv[i + 1];
    unsigned long flags = options ? parse_flags(options) : 0;

    if (sys_mount(device, target, fstype ? fstype : "", flags, NULL) < 0) {
        eprint("mount: mounting "); eprint(device);
        eprint(" on "); eprint(target); eprint(" failed\n");
        return 1;
    }
    return 0;
}

QEMT_ENTRY(mount_main)

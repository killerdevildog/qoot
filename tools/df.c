/*
 * df - disk free space (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static void pad_left(const char *s, int w) {
    int l = (int)str_len(s);
    while (l++ < w) print(" ");
    print(s);
}

static void pad_right(const char *s, int w) {
    print(s);
    int l = (int)str_len(s);
    while (l++ < w) print(" ");
}

int df_main(int argc, char **argv, char **envp) {
    (void)envp;
    int human = 0;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        human = 1;
    }

    /* Read /proc/mounts and stat each */
    pad_right("Filesystem", 20);
    if (human) {
        pad_left("Size", 8);
        pad_left("Used", 8);
        pad_left("Avail", 8);
    } else {
        pad_left("1K-blocks", 12);
        pad_left("Used", 12);
        pad_left("Available", 12);
    }
    pad_left("Use%", 6);
    print(" Mounted on\n");

    int fd = sys_open("/proc/mounts", O_RDONLY, 0);
    if (fd < 0) { eprint("df: cannot read /proc/mounts\n"); return 1; }

    char mounts[16384];
    ssize_t total = 0, n;
    while ((n = sys_read(fd, mounts + total, sizeof(mounts) - (size_t)total - 1)) > 0)
        total += n;
    mounts[total] = '\0';
    sys_close(fd);

    char *line = mounts;
    while (*line) {
        char *nl = str_chr(line, '\n');
        if (nl) *nl = '\0';

        /* Parse: device mountpoint fstype options 0 0 */
        char *dev = line;
        char *mp = str_chr(dev, ' ');
        if (!mp) { if (nl) line = nl + 1; else break; continue; }
        *mp++ = '\0';

        char *rest = str_chr(mp, ' ');
        if (rest) *rest = '\0';

        /* Skip pseudo filesystems */
        if (dev[0] != '/' && str_cmp(dev, "tmpfs") != 0 && str_cmp(dev, "devtmpfs") != 0) {
            if (nl) line = nl + 1; else break;
            continue;
        }

        struct statfs sfs;
        if (sys_statfs(mp, &sfs) < 0) {
            if (nl) line = nl + 1; else break;
            continue;
        }

        unsigned long block_size = (unsigned long)sfs.f_bsize;
        unsigned long total_blocks = (unsigned long)sfs.f_blocks;
        unsigned long free_blocks = (unsigned long)sfs.f_bfree;
        unsigned long avail_blocks = (unsigned long)sfs.f_bavail;
        unsigned long used_blocks = total_blocks - free_blocks;

        char buf[32];
        pad_right(dev, 20);

        if (human) {
            unsigned long total_bytes = total_blocks * block_size;
            unsigned long used_bytes = used_blocks * block_size;
            unsigned long avail_bytes = avail_blocks * block_size;

            /* Format in human-readable */
            const char *suffixes[] = {"B", "K", "M", "G", "T"};
            for (int pass = 0; pass < 3; pass++) {
                unsigned long val = (pass == 0) ? total_bytes : (pass == 1) ? used_bytes : avail_bytes;
                int si = 0;
                while (val >= 1024 && si < 4) { val /= 1024; si++; }
                ulong_to_str(buf, val);
                str_cat(buf, suffixes[si]);
                pad_left(buf, 8);
            }
        } else {
            unsigned long total_kb = (total_blocks * block_size) / 1024;
            unsigned long used_kb = (used_blocks * block_size) / 1024;
            unsigned long avail_kb = (avail_blocks * block_size) / 1024;

            ulong_to_str(buf, total_kb); pad_left(buf, 12);
            ulong_to_str(buf, used_kb); pad_left(buf, 12);
            ulong_to_str(buf, avail_kb); pad_left(buf, 12);
        }

        /* Use% */
        unsigned long pct = 0;
        if (total_blocks > 0) pct = (used_blocks * 100) / total_blocks;
        ulong_to_str(buf, pct); str_cat(buf, "%");
        pad_left(buf, 6);

        print(" "); print(mp); print("\n");

        if (nl) line = nl + 1; else break;
    }
    return 0;
}

QEMT_ENTRY(df_main)

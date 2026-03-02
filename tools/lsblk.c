/*
 * lsblk - list block devices (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int read_sysfs(const char *path, char *buf, int bufsize) {
    int fd = sys_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    ssize_t r = sys_read(fd, buf, (size_t)(bufsize - 1));
    sys_close(fd);
    if (r <= 0) return -1;
    /* Strip trailing newline */
    while (r > 0 && (buf[r-1] == '\n' || buf[r-1] == '\r')) r--;
    buf[r] = '\0';
    return (int)r;
}

static void format_size(unsigned long sectors, char *buf) {
    /* sectors are 512 bytes each */
    unsigned long bytes = sectors * 512;
    if (bytes >= 1024UL * 1024 * 1024 * 1024) {
        unsigned long tb = bytes / (1024UL * 1024 * 1024 * 1024);
        unsigned long frac = (bytes % (1024UL * 1024 * 1024 * 1024)) * 10 /
                             (1024UL * 1024 * 1024 * 1024);
        int off = ulong_to_str(buf, tb);
        buf[off++] = '.';
        buf[off++] = '0' + (char)frac;
        buf[off++] = 'T';
        buf[off] = '\0';
    } else if (bytes >= 1024UL * 1024 * 1024) {
        unsigned long gb = bytes / (1024UL * 1024 * 1024);
        unsigned long frac = (bytes % (1024UL * 1024 * 1024)) * 10 / (1024UL * 1024 * 1024);
        int off = ulong_to_str(buf, gb);
        buf[off++] = '.';
        buf[off++] = '0' + (char)frac;
        buf[off++] = 'G';
        buf[off] = '\0';
    } else if (bytes >= 1024UL * 1024) {
        unsigned long mb = bytes / (1024UL * 1024);
        unsigned long frac = (bytes % (1024UL * 1024)) * 10 / (1024UL * 1024);
        int off = ulong_to_str(buf, mb);
        buf[off++] = '.';
        buf[off++] = '0' + (char)frac;
        buf[off++] = 'M';
        buf[off] = '\0';
    } else if (bytes >= 1024) {
        unsigned long kb = bytes / 1024;
        int off = ulong_to_str(buf, kb);
        buf[off++] = 'K';
        buf[off] = '\0';
    } else {
        int off = ulong_to_str(buf, bytes);
        buf[off++] = 'B';
        buf[off] = '\0';
    }
}

static void pad_right(const char *s, int width) {
    print(s);
    int len = (int)str_len(s);
    while (len++ < width) print(" ");
}

static void pad_left(const char *s, int width) {
    int len = (int)str_len(s);
    while (len++ < width) print(" ");
    print(s);
}

static void print_device(const char *name, const char *prefix) {
    char path[PATH_MAX], buf[256];

    print(prefix);
    pad_right(name, 10);

    /* Major:Minor */
    char dev_str[32] = "";
    str_cpy(path, "/sys/class/block/"); str_cat(path, name); str_cat(path, "/dev");
    if (read_sysfs(path, buf, sizeof(buf)) > 0) {
        str_cpy(dev_str, buf);
    }
    pad_right(dev_str, 8);

    /* Removable */
    str_cpy(path, "/sys/class/block/"); str_cat(path, name); str_cat(path, "/removable");
    char rm = '0';
    if (read_sysfs(path, buf, sizeof(buf)) > 0 && buf[0] == '1') rm = '1';
    char rm_str[2] = {rm, '\0'};
    pad_left(rm_str, 3);
    print(" ");

    /* Size */
    str_cpy(path, "/sys/class/block/"); str_cat(path, name); str_cat(path, "/size");
    char size_str[32] = "?";
    if (read_sysfs(path, buf, sizeof(buf)) > 0) {
        unsigned long sectors = 0;
        for (int i = 0; buf[i] >= '0' && buf[i] <= '9'; i++)
            sectors = sectors * 10 + (unsigned long)(buf[i] - '0');
        format_size(sectors, size_str);
    }
    pad_left(size_str, 8);
    print(" ");

    /* RO */
    str_cpy(path, "/sys/class/block/"); str_cat(path, name); str_cat(path, "/ro");
    char ro = '0';
    if (read_sysfs(path, buf, sizeof(buf)) > 0 && buf[0] == '1') ro = '1';
    char ro_str[2] = {ro, '\0'};
    pad_left(ro_str, 3);
    print(" ");

    /* Type */
    str_cpy(path, "/sys/class/block/"); str_cat(path, name); str_cat(path, "/device");
    struct stat st;
    const char *type = "part";
    /* If it has a device/ subdir and no partition number, it's a disk */
    str_cpy(path, "/sys/class/block/"); str_cat(path, name); str_cat(path, "/partition");
    if (sys_stat(path, &st) < 0) type = "disk";

    pad_right(type, 6);

    /* Mountpoint from /proc/mounts */
    {
        char dev_path[PATH_MAX];
        str_cpy(dev_path, "/dev/"); str_cat(dev_path, name);
        int mfd = sys_open("/proc/mounts", O_RDONLY, 0);
        if (mfd >= 0) {
            char line[512];
            while (read_line(mfd, line, sizeof(line)) > 0) {
                if (starts_with(line, dev_path) &&
                    line[str_len(dev_path)] == ' ') {
                    char *mp = line + str_len(dev_path) + 1;
                    char *end = str_chr(mp, ' ');
                    if (end) *end = '\0';
                    print(mp);
                    break;
                }
            }
            sys_close(mfd);
        }
    }

    print("\n");
}

int lsblk_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && (str_cmp(argv[1], "-h") == 0 || str_cmp(argv[1], "--help") == 0)) {
        print("Usage: lsblk\n");
        print("List block devices.\n");
        return 0;
    }

    /* Header */
    pad_right("NAME", 10);
    pad_right("MAJ:MIN", 8);
    pad_left("RM", 3); print(" ");
    pad_left("SIZE", 8); print(" ");
    pad_left("RO", 3); print(" ");
    pad_right("TYPE", 6);
    print("MOUNTPOINT");
    print("\n");

    /* Enumerate /sys/class/block/ */
    int dfd = sys_open("/sys/class/block", O_RDONLY | O_DIRECTORY, 0);
    if (dfd < 0) { eprint("lsblk: cannot read /sys/class/block\n"); return 1; }

    /* First pass: collect parent disks */
    char buf[4096];
    int nread;
    char disks[64][NAME_MAX];
    int ndisks = 0;

    while ((nread = sys_getdents64(dfd, buf, sizeof(buf))) > 0) {
        int pos = 0;
        while (pos < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);
            if (d->d_name[0] != '.') {
                char ppath[PATH_MAX];
                str_cpy(ppath, "/sys/class/block/");
                str_cat(ppath, d->d_name);
                str_cat(ppath, "/partition");
                struct stat st;
                if (sys_stat(ppath, &st) < 0 && ndisks < 64) {
                    /* No partition file = this is a disk */
                    str_cpy(disks[ndisks++], d->d_name);
                }
            }
            pos += d->d_reclen;
        }
    }
    sys_close(dfd);

    /* For each disk, print it and its partitions */
    for (int i = 0; i < ndisks; i++) {
        print_device(disks[i], "");

        /* Find partitions */
        dfd = sys_open("/sys/class/block", O_RDONLY | O_DIRECTORY, 0);
        if (dfd < 0) continue;

        while ((nread = sys_getdents64(dfd, buf, sizeof(buf))) > 0) {
            int pos = 0;
            while (pos < nread) {
                struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);
                if (starts_with(d->d_name, disks[i]) &&
                    str_cmp(d->d_name, disks[i]) != 0) {
                    print_device(d->d_name, "├─");
                }
                pos += d->d_reclen;
            }
        }
        sys_close(dfd);
    }

    return 0;
}

QEMT_ENTRY(lsblk_main)

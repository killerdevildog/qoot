/*
 * ps - process status (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static void pad_right(const char *s, int w) {
    print(s);
    int l = (int)str_len(s);
    while (l++ < w) print(" ");
}

static void pad_left(const char *s, int w) {
    int l = (int)str_len(s);
    while (l++ < w) print(" ");
    print(s);
}

static int read_proc_file(const char *path, char *buf, size_t sz) {
    int fd = sys_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    ssize_t n = sys_read(fd, buf, sz - 1);
    sys_close(fd);
    if (n < 0) return -1;
    buf[n] = '\0';
    return (int)n;
}

int ps_main(int argc, char **argv, char **envp) {
    (void)envp;
    int show_all = 0;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: ps [-e|-A|aux]\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-e") == 0 || str_cmp(argv[i], "-A") == 0 ||
            str_cmp(argv[i], "aux") == 0 || str_cmp(argv[i], "-a") == 0)
            show_all = 1;
    }

    /* Header */
    pad_left("PID", 7); print(" ");
    pad_left("PPID", 7); print(" ");
    pad_right("STATE", 6);
    pad_right("USER", 10);
    print("COMMAND\n");

    int fd = sys_open("/proc", O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) { eprint("ps: cannot open /proc\n"); return 1; }

    uid_t my_uid = sys_getuid();
    char buf[8192];
    int nread;

    while ((nread = sys_getdents64(fd, buf, sizeof(buf))) > 0) {
        int pos = 0;
        while (pos < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);

            /* Only process numeric directories (PIDs) */
            int is_pid = 1;
            for (int j = 0; d->d_name[j]; j++) {
                if (!is_digit(d->d_name[j])) { is_pid = 0; break; }
            }
            if (!is_pid) { pos += d->d_reclen; continue; }

            char status_path[64];
            str_cpy(status_path, "/proc/");
            str_cat(status_path, d->d_name);
            str_cat(status_path, "/status");

            char sbuf[4096];
            if (read_proc_file(status_path, sbuf, sizeof(sbuf)) < 0) {
                pos += d->d_reclen;
                continue;
            }

            /* Parse status fields */
            char pname[256] = "", state[4] = "?", ppid[16] = "0", puid[16] = "0";
            char *line = sbuf;
            while (*line) {
                if (starts_with(line, "Name:\t")) {
                    char *val = line + 6;
                    int k = 0;
                    while (val[k] && val[k] != '\n' && k < 255) { pname[k] = val[k]; k++; }
                    pname[k] = '\0';
                } else if (starts_with(line, "State:\t")) {
                    state[0] = line[7]; state[1] = '\0';
                } else if (starts_with(line, "PPid:\t")) {
                    char *val = line + 6;
                    int k = 0;
                    while (val[k] && val[k] != '\n' && k < 15) { ppid[k] = val[k]; k++; }
                    ppid[k] = '\0';
                } else if (starts_with(line, "Uid:\t")) {
                    char *val = line + 5;
                    int k = 0;
                    while (val[k] && val[k] != '\t' && val[k] != '\n' && k < 15) { puid[k] = val[k]; k++; }
                    puid[k] = '\0';
                }
                /* Advance to next line */
                while (*line && *line != '\n') line++;
                if (*line == '\n') line++;
            }

            uid_t proc_uid = (uid_t)str_to_uint(puid);
            if (!show_all && proc_uid != my_uid) { pos += d->d_reclen; continue; }

            /* Get command line */
            char cmdline_path[64];
            str_cpy(cmdline_path, "/proc/");
            str_cat(cmdline_path, d->d_name);
            str_cat(cmdline_path, "/cmdline");

            char cmd[512] = "";
            int cfd = sys_open(cmdline_path, O_RDONLY, 0);
            if (cfd >= 0) {
                ssize_t cn = sys_read(cfd, cmd, sizeof(cmd) - 1);
                sys_close(cfd);
                if (cn > 0) {
                    cmd[cn] = '\0';
                    /* Replace null bytes with spaces */
                    for (ssize_t j = 0; j < cn - 1; j++)
                        if (cmd[j] == '\0') cmd[j] = ' ';
                }
            }
            if (cmd[0] == '\0') {
                str_cpy(cmd, "[");
                str_cat(cmd, pname);
                str_cat(cmd, "]");
            }

            char owner[64];
            if (uid_to_name(proc_uid, owner, sizeof(owner)) != 0) {
                str_cpy(owner, puid);
            }

            pad_left(d->d_name, 7); print(" ");
            pad_left(ppid, 7); print(" ");
            pad_right(state, 6);
            pad_right(owner, 10);
            print(cmd); print("\n");

            pos += d->d_reclen;
        }
    }
    sys_close(fd);
    return 0;
}

QEMT_ENTRY(ps_main)

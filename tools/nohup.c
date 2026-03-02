/*
 * nohup - run command immune to hangups (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

/* Minimal signal handling via raw syscall */
#define SYS_rt_sigaction 13
#define SA_RESTORER 0x04000000

struct kernel_sigaction {
    void (*handler)(int);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

static void sig_ignore(int sig) { (void)sig; }

int nohup_main(int argc, char **argv, char **envp) {
    if (argc < 2) {
        print("Usage: nohup COMMAND [ARGS...]\n");
        print("Run COMMAND immune to hangups, with output to nohup.out\n");
        return 1;
    }

    /* Ignore SIGHUP */
    struct kernel_sigaction sa;
    mem_set(&sa, 0, sizeof(sa));
    sa.handler = sig_ignore;
    syscall4(SYS_rt_sigaction, SIGHUP, (long)&sa, 0, 8);

    /* Redirect stdout to nohup.out if it's a terminal */
    char tty_check[256];
    ssize_t n = sys_readlink("/proc/self/fd/1", tty_check, sizeof(tty_check) - 1);
    if (n > 0) {
        tty_check[n] = '\0';
        if (starts_with(tty_check, "/dev/pts/") || starts_with(tty_check, "/dev/tty")) {
            int fd = sys_open("nohup.out", O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd >= 0) {
                sys_dup2(fd, 1);
                sys_dup2(fd, 2);
                sys_close(fd);
                eprint("nohup: appending output to nohup.out\n");
            }
        }
    }

    sys_execve(argv[1], argv + 1, envp);

    /* Try PATH lookup */
    char path[PATH_MAX];
    const char *dirs[] = {"/usr/local/bin/", "/usr/bin/", "/bin/", "/usr/sbin/", "/sbin/", NULL};
    for (int i = 0; dirs[i]; i++) {
        str_cpy(path, dirs[i]);
        str_cat(path, argv[1]);
        sys_execve(path, argv + 1, envp);
    }

    eprint("nohup: '"); eprint(argv[1]); eprint("': No such file\n");
    return 127;
}

QEMT_ENTRY(nohup_main)

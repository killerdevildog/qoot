/*
 * xargs - build and execute commands from stdin (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int xargs_main(int argc, char **argv, char **envp) {
    const char *cmd = "/bin/echo";
    int max_args = 100;
    int argi = 1;

    while (argi < argc && argv[argi][0] == '-') {
        if (str_cmp(argv[argi], "-n") == 0 && argi + 1 < argc) {
            max_args = (int)str_to_uint(argv[++argi]);
            argi++;
        } else if (str_cmp(argv[argi], "-h") == 0 || str_cmp(argv[argi], "--help") == 0) {
            print("Usage: xargs [-n MAX] [command [args...]]\n");
            return 0;
        } else {
            break;
        }
    }

    /* Remaining args are the command + initial args */
    int cmd_argc = 0;
    char *cmd_argv[256];

    if (argi < argc) {
        cmd = argv[argi];
        for (int i = argi; i < argc && cmd_argc < 200; i++)
            cmd_argv[cmd_argc++] = argv[i];
    } else {
        cmd_argv[cmd_argc++] = (char *)"echo";
    }
    int base_argc = cmd_argc;

    /* Read args from stdin */
    char buf[LINE_MAX];
    int len;

    while ((len = read_line(0, buf, LINE_MAX)) > 0) {
        /* Split line by whitespace */
        char *p = buf;
        while (*p) {
            while (*p && is_space(*p)) p++;
            if (!*p) break;

            char *start = p;
            while (*p && !is_space(*p)) p++;
            if (*p) *p++ = '\0';

            if (cmd_argc < 255)
                cmd_argv[cmd_argc++] = start;

            if (cmd_argc - base_argc >= max_args) {
                /* Execute batch */
                cmd_argv[cmd_argc] = NULL;
                pid_t pid = sys_fork();
                if (pid == 0) {
                    sys_execve(cmd, cmd_argv, envp);
                    /* Try PATH search */
                    char path[PATH_MAX];
                    str_cpy(path, "/usr/bin/");
                    str_cat(path, cmd);
                    sys_execve(path, cmd_argv, envp);
                    str_cpy(path, "/bin/");
                    str_cat(path, cmd);
                    sys_execve(path, cmd_argv, envp);
                    eprint("xargs: "); eprint(cmd); eprint(": not found\n");
                    sys_exit(127);
                }
                if (pid > 0) {
                    int status;
                    sys_wait4(pid, &status, 0, NULL);
                }
                cmd_argc = base_argc;
            }
        }
    }

    /* Execute remaining */
    if (cmd_argc > base_argc) {
        cmd_argv[cmd_argc] = NULL;
        pid_t pid = sys_fork();
        if (pid == 0) {
            sys_execve(cmd, cmd_argv, envp);
            char path[PATH_MAX];
            str_cpy(path, "/usr/bin/");
            str_cat(path, cmd);
            sys_execve(path, cmd_argv, envp);
            str_cpy(path, "/bin/");
            str_cat(path, cmd);
            sys_execve(path, cmd_argv, envp);
            eprint("xargs: "); eprint(cmd); eprint(": not found\n");
            sys_exit(127);
        }
        if (pid > 0) {
            int status;
            sys_wait4(pid, &status, 0, NULL);
        }
    }

    return 0;
}

QEMT_ENTRY(xargs_main)

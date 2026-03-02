/*
 * kill - send signal to process (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int parse_signal(const char *s) {
    if (is_digit(s[0])) return (int)str_to_uint(s);
    if (str_cmp(s, "HUP") == 0 || str_cmp(s, "SIGHUP") == 0) return 1;
    if (str_cmp(s, "INT") == 0 || str_cmp(s, "SIGINT") == 0) return 2;
    if (str_cmp(s, "QUIT") == 0 || str_cmp(s, "SIGQUIT") == 0) return 3;
    if (str_cmp(s, "KILL") == 0 || str_cmp(s, "SIGKILL") == 0) return 9;
    if (str_cmp(s, "TERM") == 0 || str_cmp(s, "SIGTERM") == 0) return 15;
    if (str_cmp(s, "STOP") == 0 || str_cmp(s, "SIGSTOP") == 0) return 19;
    if (str_cmp(s, "CONT") == 0 || str_cmp(s, "SIGCONT") == 0) return 18;
    if (str_cmp(s, "USR1") == 0 || str_cmp(s, "SIGUSR1") == 0) return 10;
    if (str_cmp(s, "USR2") == 0 || str_cmp(s, "SIGUSR2") == 0) return 12;
    return -1;
}

int kill_main(int argc, char **argv, char **envp) {
    (void)envp;
    int sig = SIGTERM;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: kill [-signal] pid...\n");
        print("Signals: HUP INT QUIT KILL TERM STOP CONT USR1 USR2 (or number)\n");
        return 0;
    }

    if (argc > 1 && str_cmp(argv[1], "-l") == 0) {
        print(" 1) SIGHUP    2) SIGINT    3) SIGQUIT   9) SIGKILL\n");
        print("10) SIGUSR1  12) SIGUSR2  15) SIGTERM  18) SIGCONT\n");
        print("19) SIGSTOP\n");
        return 0;
    }

    if (argc > 1 && argv[1][0] == '-' && !is_digit(argv[1][1]) && argv[1][1] != '\0') {
        sig = parse_signal(argv[1] + 1);
        if (sig < 0) {
            eprint("kill: unknown signal '"); eprint(argv[1] + 1); eprint("'\n");
            return 1;
        }
        first = 2;
    } else if (argc > 2 && str_cmp(argv[1], "-s") == 0) {
        sig = parse_signal(argv[2]);
        if (sig < 0) {
            eprint("kill: unknown signal '"); eprint(argv[2]); eprint("'\n");
            return 1;
        }
        first = 3;
    } else if (argc > 1 && argv[1][0] == '-' && is_digit(argv[1][1])) {
        sig = (int)str_to_uint(argv[1] + 1);
        first = 2;
    }

    if (first >= argc) {
        eprint("kill: missing operand\n");
        return 1;
    }

    int ret = 0;
    for (int i = first; i < argc; i++) {
        long pid = str_to_long(argv[i]);
        if (sys_kill((int)pid, sig) < 0) {
            eprint("kill: ("); eprint(argv[i]); eprint(") - No such process\n");
            ret = 1;
        }
    }
    return ret;
}

QEMT_ENTRY(kill_main)

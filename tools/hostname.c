/*
 * hostname - show or set system hostname (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int hostname_main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: hostname [new_hostname]\n");
        return 0;
    }

    if (argc > 1) {
        /* Set hostname (requires root) */
        if (sys_sethostname(argv[1], str_len(argv[1])) < 0) {
            eprint("hostname: cannot set hostname (permission denied?)\n");
            return 1;
        }
        return 0;
    }

    struct utsname uts;
    if (sys_uname(&uts) < 0) {
        eprint("hostname: cannot get hostname\n");
        return 1;
    }
    print(uts.nodename);
    print("\n");
    return 0;
}

QEMT_ENTRY(hostname_main)

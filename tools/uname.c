/*
 * uname - print system information (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int uname_main(int argc, char **argv, char **envp) {
    (void)envp;
    struct utsname uts;
    if (sys_uname(&uts) < 0) {
        eprint("uname: cannot get system info\n");
        return 1;
    }

    int all = 0, s = 0, n = 0, r = 0, v = 0, m = 0;

    if (argc <= 1) { s = 1; }
    else {
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                for (int j = 1; argv[i][j]; j++) {
                    switch (argv[i][j]) {
                        case 'a': all = 1; break;
                        case 's': s = 1; break;
                        case 'n': n = 1; break;
                        case 'r': r = 1; break;
                        case 'v': v = 1; break;
                        case 'm': m = 1; break;
                        default:
                            eprint("Usage: uname [-asnrvm]\n");
                            return 1;
                    }
                }
            }
        }
    }

    if (all) { s = n = r = v = m = 1; }

    int printed = 0;
    if (s) { print(uts.sysname); printed = 1; }
    if (n) { if (printed) print(" "); print(uts.nodename); printed = 1; }
    if (r) { if (printed) print(" "); print(uts.release); printed = 1; }
    if (v) { if (printed) print(" "); print(uts.version); printed = 1; }
    if (m) { if (printed) print(" "); print(uts.machine); }
    print("\n");
    return 0;
}

QEMT_ENTRY(uname_main)

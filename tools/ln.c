/*
 * ln - create links (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int ln_main(int argc, char **argv, char **envp) {
    (void)envp;
    int symbolic = 0, force = 0;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: ln [-sf] target link_name\n");
        print("  -s  Create symbolic link\n  -f  Force (remove existing)\n");
        return 0;
    }

    while (first < argc && argv[first][0] == '-' && argv[first][1] != '\0') {
        for (int j = 1; argv[first][j]; j++) {
            if (argv[first][j] == 's') symbolic = 1;
            else if (argv[first][j] == 'f') force = 1;
            else { eprint("ln: unknown option\n"); return 1; }
        }
        first++;
    }

    if (argc - first < 2) {
        eprint("ln: missing operand\nUsage: ln [-sf] target link_name\n");
        return 1;
    }

    const char *target = argv[first];
    const char *link_name = argv[first + 1];

    if (force) sys_unlink(link_name);

    int r;
    if (symbolic)
        r = sys_symlink(target, link_name);
    else
        r = sys_link(target, link_name);

    if (r < 0) {
        eprint("ln: failed to create link '"); eprint(link_name); eprint("'\n");
        return 1;
    }
    return 0;
}

QEMT_ENTRY(ln_main)

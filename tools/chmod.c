/*
 * chmod - change file permissions (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int chmod_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: chmod mode file...\n");
        print("  mode: octal permission (e.g., 755)\n");
        return 0;
    }

    if (argc < 3) {
        eprint("chmod: missing operand\nUsage: chmod mode file...\n");
        return 1;
    }

    unsigned int mode = oct_to_uint(argv[1]);
    if (mode > 07777) {
        eprint("chmod: invalid mode '"); eprint(argv[1]); eprint("'\n");
        return 1;
    }

    int ret = 0;
    for (int i = 2; i < argc; i++) {
        if (sys_chmod(argv[i], mode) < 0) {
            eprint("chmod: cannot change '"); eprint(argv[i]); eprint("'\n");
            ret = 1;
        }
    }
    return ret;
}

QEMT_ENTRY(chmod_main)

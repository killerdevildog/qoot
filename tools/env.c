/*
 * env - print environment variables (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int env_main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv;
    for (int i = 0; envp[i]; i++) {
        print(envp[i]);
        print("\n");
    }
    return 0;
}

QEMT_ENTRY(env_main)

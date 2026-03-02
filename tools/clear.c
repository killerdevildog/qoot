/*
 * clear - clear terminal screen (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int clear_main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;
    print("\033[H\033[2J\033[3J");
    return 0;
}

QEMT_ENTRY(clear_main)

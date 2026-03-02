/*
 * true - exit with success status (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int true_main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;
    return 0;
}

QEMT_ENTRY(true_main)

/*
 * echo - print arguments (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int echo_main(int argc, char **argv, char **envp) {
    (void)envp;
    int newline = 1;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "-n") == 0) {
        newline = 0;
        first = 2;
    }

    for (int i = first; i < argc; i++) {
        if (i > first) print(" ");
        print(argv[i]);
    }
    if (newline) print("\n");
    return 0;
}

QEMT_ENTRY(echo_main)

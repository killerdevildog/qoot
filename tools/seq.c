/*
 * seq - print number sequences (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int seq_main(int argc, char **argv, char **envp) {
    (void)envp;
    long first = 1, inc = 1, last = 0;
    const char *sep = "\n";

    int argi = 1;
    while (argi < argc && argv[argi][0] == '-' && argv[argi][1]) {
        if (str_cmp(argv[argi], "-s") == 0 && argi + 1 < argc) {
            sep = argv[++argi]; argi++;
        } else if (str_cmp(argv[argi], "-h") == 0 || str_cmp(argv[argi], "--help") == 0) {
            print("Usage: seq [FIRST [INC]] LAST\n");
            print("       seq -s SEP [FIRST [INC]] LAST\n");
            return 0;
        } else {
            break; /* negative number */
        }
    }

    int remaining = argc - argi;
    if (remaining == 1) {
        last = str_to_long(argv[argi]);
    } else if (remaining == 2) {
        first = str_to_long(argv[argi]);
        last = str_to_long(argv[argi + 1]);
    } else if (remaining >= 3) {
        first = str_to_long(argv[argi]);
        inc = str_to_long(argv[argi + 1]);
        last = str_to_long(argv[argi + 2]);
    } else {
        eprint("seq: missing operand\n");
        return 1;
    }

    if (inc == 0) { eprint("seq: zero increment\n"); return 1; }

    char buf[21];
    int started = 0;
    for (long i = first; (inc > 0 ? i <= last : i >= last); i += inc) {
        if (started) print(sep);
        long_to_str(buf, i);
        print(buf);
        started = 1;
    }
    if (started) print("\n");
    return 0;
}

QEMT_ENTRY(seq_main)

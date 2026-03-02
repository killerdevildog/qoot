/*
 * whoami - print effective user name (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int whoami_main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;
    uid_t uid = sys_geteuid();
    char name[256];
    if (uid_to_name(uid, name, sizeof(name)) != 0) {
        print(name);
    } else {
        char buf[21];
        uint_to_str(buf, uid);
        print(buf);
    }
    print("\n");
    return 0;
}

QEMT_ENTRY(whoami_main)

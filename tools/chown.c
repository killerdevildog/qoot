/*
 * chown - change file ownership (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int chown_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: chown owner[:group] file...\n");
        return 0;
    }

    if (argc < 3) {
        eprint("chown: missing operand\nUsage: chown owner[:group] file...\n");
        return 1;
    }

    /* Parse owner:group */
    char owner_str[256], group_str[256];
    const char *spec = argv[1];
    const char *colon = str_chr(spec, ':');

    uid_t uid = (uid_t)-1;
    gid_t gid = (gid_t)-1;

    if (colon) {
        size_t olen = (size_t)(colon - spec);
        if (olen >= sizeof(owner_str)) olen = sizeof(owner_str) - 1;
        mem_cpy(owner_str, spec, olen);
        owner_str[olen] = '\0';
        str_cpy(group_str, colon + 1);
    } else {
        str_cpy(owner_str, spec);
        group_str[0] = '\0';
    }

    if (owner_str[0]) {
        if (is_digit(owner_str[0])) {
            uid = (uid_t)str_to_uint(owner_str);
        } else {
            if (name_to_uid(owner_str, &uid) != 0) {
                eprint("chown: unknown user '"); eprint(owner_str); eprint("'\n");
                return 1;
            }
        }
    }

    if (group_str[0]) {
        if (is_digit(group_str[0])) {
            gid = (gid_t)str_to_uint(group_str);
        } else {
            if (name_to_gid(group_str, &gid) != 0) {
                eprint("chown: unknown group '"); eprint(group_str); eprint("'\n");
                return 1;
            }
        }
    }

    int ret = 0;
    for (int i = 2; i < argc; i++) {
        if (sys_chown(argv[i], uid, gid) < 0) {
            eprint("chown: cannot change '"); eprint(argv[i]); eprint("'\n");
            ret = 1;
        }
    }
    return ret;
}

QEMT_ENTRY(chown_main)

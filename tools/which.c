/*
 * which - locate a command in PATH (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static char *get_env(char **envp, const char *name) {
    size_t nlen = str_len(name);
    for (int i = 0; envp[i]; i++) {
        if (str_ncmp(envp[i], name, nlen) == 0 && envp[i][nlen] == '=')
            return envp[i] + nlen + 1;
    }
    return NULL;
}

int which_main(int argc, char **argv, char **envp) {
    if (argc < 2) {
        eprint("Usage: which command...\n");
        return 1;
    }

    char *path = get_env(envp, "PATH");
    if (!path) path = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";

    int ret = 0;
    for (int i = 1; i < argc; i++) {
        /* If it contains a slash, check directly */
        if (str_chr(argv[i], '/')) {
            if (sys_access(argv[i], 1) == 0) { /* X_OK = 1 */
                print(argv[i]); print("\n");
                continue;
            }
            ret = 1;
            continue;
        }

        int found = 0;
        char pcopy[4096];
        str_ncpy(pcopy, path, sizeof(pcopy) - 1);
        pcopy[sizeof(pcopy) - 1] = '\0';

        char *p = pcopy;
        while (*p) {
            char *colon = str_chr(p, ':');
            if (colon) *colon = '\0';

            char full[PATH_MAX];
            str_cpy(full, p);
            str_cat(full, "/");
            str_cat(full, argv[i]);

            if (sys_access(full, 1) == 0) { /* X_OK = 1 */
                print(full); print("\n");
                found = 1;
                break;
            }

            if (colon) p = colon + 1;
            else break;
        }

        if (!found) {
            eprint(argv[i]); eprint(" not found\n");
            ret = 1;
        }
    }
    return ret;
}

QEMT_ENTRY(which_main)

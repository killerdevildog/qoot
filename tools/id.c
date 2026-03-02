/*
 * id - print user and group IDs (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int id_main(int argc, char **argv, char **envp) {
    (void)envp;
    uid_t uid = sys_getuid();
    uid_t euid = sys_geteuid();
    gid_t gid = sys_getgid();
    gid_t egid = sys_getegid();
    char name[256], buf[21];

    if (argc > 1 && str_cmp(argv[1], "-u") == 0) {
        uint_to_str(buf, euid);
        print(buf); print("\n");
        return 0;
    }
    if (argc > 1 && str_cmp(argv[1], "-g") == 0) {
        uint_to_str(buf, egid);
        print(buf); print("\n");
        return 0;
    }
    if (argc > 1 && str_cmp(argv[1], "-n") == 0) {
        if (uid_to_name(euid, name, sizeof(name)) == 0) print(name);
        else { uint_to_str(buf, euid); print(buf); }
        print("\n");
        return 0;
    }

    /* uid=X(name) gid=X(name) euid=X(name) groups=... */
    print("uid=");
    uint_to_str(buf, uid); print(buf);
    print("(");
    if (uid_to_name(uid, name, sizeof(name)) == 0) print(name);
    print(")");

    print(" gid=");
    uint_to_str(buf, gid); print(buf);
    print("(");
    if (gid_to_name(gid, name, sizeof(name)) == 0) print(name);
    print(")");

    if (euid != uid) {
        print(" euid=");
        uint_to_str(buf, euid); print(buf);
        print("(");
        if (uid_to_name(euid, name, sizeof(name)) == 0) print(name);
        print(")");
    }

    if (egid != gid) {
        print(" egid=");
        uint_to_str(buf, egid); print(buf);
        print("(");
        if (gid_to_name(egid, name, sizeof(name)) == 0) print(name);
        print(")");
    }

    /* Print supplementary groups */
    gid_t groups[64];
    int ngroups = sys_getgroups(64, groups);
    if (ngroups > 0) {
        print(" groups=");
        for (int i = 0; i < ngroups; i++) {
            if (i > 0) print(",");
            uint_to_str(buf, groups[i]); print(buf);
            print("(");
            if (gid_to_name(groups[i], name, sizeof(name)) == 0) print(name);
            print(")");
        }
    }
    print("\n");
    return 0;
}

QEMT_ENTRY(id_main)

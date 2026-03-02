/*
 * stat - display file status (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static const char *file_type(unsigned int mode) {
    if (S_ISREG(mode))  return "regular file";
    if (S_ISDIR(mode))  return "directory";
    if (S_ISLNK(mode))  return "symbolic link";
    if (S_ISCHR(mode))  return "character device";
    if (S_ISBLK(mode))  return "block device";
    if (S_ISFIFO(mode)) return "FIFO/pipe";
    if (S_ISSOCK(mode)) return "socket";
    return "unknown";
}

int stat_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc < 2 || str_cmp(argv[1], "-h") == 0) {
        print("Usage: stat file...\n");
        return argc < 2 ? 1 : 0;
    }

    char buf[21], mode_str[11];
    char name[256];

    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (sys_lstat(argv[i], &st) < 0) {
            eprint("stat: cannot stat '"); eprint(argv[i]); eprint("'\n");
            continue;
        }

        print("  File: "); print(argv[i]);
        if (S_ISLNK(st.st_mode)) {
            char link[PATH_MAX];
            ssize_t n = sys_readlink(argv[i], link, PATH_MAX - 1);
            if (n > 0) { link[n] = '\0'; print(" -> "); print(link); }
        }
        print("\n");

        print("  Size: "); long_to_str(buf, st.st_size); print(buf);
        print("\tBlocks: "); long_to_str(buf, st.st_blocks); print(buf);
        print("\t"); print(file_type(st.st_mode)); print("\n");

        format_mode(st.st_mode, mode_str);
        print("Access: (0");
        uint_to_oct(buf, st.st_mode & 07777); print(buf);
        print("/"); print(mode_str); print(")");

        print("  Uid: ("); uint_to_str(buf, st.st_uid); print(buf); print("/");
        if (uid_to_name(st.st_uid, name, sizeof(name)) == 0) print(name);
        else print("?");
        print(")");

        print("  Gid: ("); uint_to_str(buf, st.st_gid); print(buf); print("/");
        if (gid_to_name(st.st_gid, name, sizeof(name)) == 0) print(name);
        else print("?");
        print(")\n");

        print("Device: "); ulong_to_str(buf, st.st_dev); print(buf);
        print("\tInode: "); ulong_to_str(buf, st.st_ino); print(buf);
        print("\tLinks: "); ulong_to_str(buf, st.st_nlink); print(buf);
        print("\n");

        if (i < argc - 1) print("\n");
    }
    return 0;
}

QEMT_ENTRY(stat_main)

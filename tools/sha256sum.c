/*
 * sha256sum - compute SHA-256 hash (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/sha256.h"
#include "../include/io.h"

static const char hex[] = "0123456789abcdef";

static void hash_fd(int fd, const char *name) {
    sha256_ctx_t ctx;
    sha256_init(&ctx);

    unsigned char buf[4096];
    ssize_t r;
    while ((r = sys_read(fd, buf, sizeof(buf))) > 0)
        sha256_update(&ctx, buf, (unsigned long)r);

    unsigned char digest[32];
    sha256_final(&ctx, digest);

    char out[65];
    for (int i = 0; i < 32; i++) {
        out[i * 2] = hex[digest[i] >> 4];
        out[i * 2 + 1] = hex[digest[i] & 0xf];
    }
    out[64] = '\0';
    print(out);
    print("  ");
    print(name);
    print("\n");
}

int sha256sum_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && (str_cmp(argv[1], "-h") == 0 || str_cmp(argv[1], "--help") == 0)) {
        print("Usage: sha256sum [file...]\n");
        return 0;
    }

    if (argc < 2) {
        hash_fd(0, "-");
    } else {
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-' && argv[i][1] == '\0') {
                hash_fd(0, "-");
                continue;
            }
            int fd = sys_open(argv[i], O_RDONLY, 0);
            if (fd < 0) {
                eprint("sha256sum: "); eprint(argv[i]); eprint(": No such file\n");
                continue;
            }
            hash_fd(fd, argv[i]);
            sys_close(fd);
        }
    }
    return 0;
}

QEMT_ENTRY(sha256sum_main)

/*
 * ar - archive tool for ar format (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 * Useful for extracting .deb packages (which are ar archives).
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

#define AR_MAGIC "!<arch>\n"
#define AR_MAGIC_LEN 8

/* ar file header (60 bytes) */
struct ar_header {
    char ar_name[16];
    char ar_date[12];
    char ar_uid[6];
    char ar_gid[6];
    char ar_mode[8];
    char ar_size[10];
    char ar_fmag[2];
};

static unsigned long ar_parse_size(const char *s, int maxlen) {
    unsigned long val = 0;
    for (int i = 0; i < maxlen && s[i] >= '0' && s[i] <= '9'; i++)
        val = val * 10 + (unsigned long)(s[i] - '0');
    return val;
}

static void ar_get_name(const struct ar_header *h, char *out, int maxout) {
    int i;
    for (i = 0; i < 16 && i < maxout - 1; i++) {
        if (h->ar_name[i] == '/' || h->ar_name[i] == ' ') break;
        out[i] = h->ar_name[i];
    }
    out[i] = '\0';
}

static int ar_list(int fd) {
    char magic[AR_MAGIC_LEN];
    if (sys_read(fd, magic, AR_MAGIC_LEN) != AR_MAGIC_LEN ||
        mem_cmp(magic, AR_MAGIC, AR_MAGIC_LEN) != 0) {
        eprint("ar: not an ar archive\n");
        return 1;
    }

    struct ar_header h;
    while (sys_read(fd, &h, 60) == 60) {
        if (h.ar_fmag[0] != '`' || h.ar_fmag[1] != '\n') break;

        char name[64];
        ar_get_name(&h, name, sizeof(name));
        unsigned long size = ar_parse_size(h.ar_size, 10);

        print(name); print("\n");

        /* Skip data + padding */
        unsigned long skip = size + (size & 1);
        sys_lseek(fd, (off_t)skip, SEEK_CUR);
    }
    return 0;
}

static int ar_extract(int fd, int verbose) {
    char magic[AR_MAGIC_LEN];
    if (sys_read(fd, magic, AR_MAGIC_LEN) != AR_MAGIC_LEN ||
        mem_cmp(magic, AR_MAGIC, AR_MAGIC_LEN) != 0) {
        eprint("ar: not an ar archive\n");
        return 1;
    }

    struct ar_header h;
    while (sys_read(fd, &h, 60) == 60) {
        if (h.ar_fmag[0] != '`' || h.ar_fmag[1] != '\n') break;

        char name[64];
        ar_get_name(&h, name, sizeof(name));
        unsigned long size = ar_parse_size(h.ar_size, 10);

        if (verbose) { print(name); print("\n"); }

        int out = sys_open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out < 0) {
            eprint("ar: cannot create '"); eprint(name); eprint("'\n");
            sys_lseek(fd, (off_t)(size + (size & 1)), SEEK_CUR);
            continue;
        }

        unsigned long remaining = size;
        char buf[4096];
        while (remaining > 0) {
            size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : (size_t)remaining;
            ssize_t r = sys_read(fd, buf, chunk);
            if (r <= 0) break;
            write_all(out, buf, (size_t)r);
            remaining -= (unsigned long)r;
        }
        sys_close(out);

        /* Skip padding byte if size is odd */
        if (size & 1) sys_lseek(fd, 1, SEEK_CUR);
    }
    return 0;
}

int ar_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc < 3) {
        print("Usage: ar [tx] archive\n");
        print("  t   List contents\n");
        print("  x   Extract files\n");
        print("\nUseful for extracting .deb packages:\n");
        print("  ar x package.deb\n");
        print("  gunzip data.tar.gz\n");
        print("  tar xf data.tar\n");
        return 1;
    }

    const char *op = argv[1];
    const char *file = argv[2];
    int verbose = 0;

    /* Check for 'v' in operation string */
    for (int i = 0; op[i]; i++)
        if (op[i] == 'v') verbose = 1;

    int fd = sys_open(file, O_RDONLY, 0);
    if (fd < 0) {
        eprint("ar: cannot open '"); eprint(file); eprint("'\n");
        return 1;
    }

    int ret = 1;
    for (int i = 0; op[i]; i++) {
        if (op[i] == 't') { ret = ar_list(fd); break; }
        if (op[i] == 'x') { ret = ar_extract(fd, verbose); break; }
    }

    sys_close(fd);
    return ret;
}

QEMT_ENTRY(ar_main)

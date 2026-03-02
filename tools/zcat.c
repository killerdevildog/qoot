/*
 * zcat - decompress .gz files to stdout (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/deflate.h"
#include "../include/io.h"

int zcat_main(int argc, char **argv, char **envp) {
    (void)envp;
    int nfiles = 0;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "--help") == 0 || str_cmp(argv[i], "-h") == 0) {
            print("Usage: zcat [file.gz ...]\n");
            print("Decompress .gz files to stdout.\n");
            print("With no files, reads from stdin.\n");
            return 0;
        }
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') continue;

        int fd = sys_open(argv[i], O_RDONLY, 0);
        if (fd < 0) {
            eprint("zcat: "); eprint(argv[i]); eprint(": No such file\n");
            continue;
        }

        struct stat st;
        if (sys_fstat(fd, &st) < 0 || st.st_size < 18) {
            eprint("zcat: "); eprint(argv[i]); eprint(": not in gzip format\n");
            sys_close(fd);
            continue;
        }

        unsigned char *data = (unsigned char *)sys_mmap(NULL, (size_t)st.st_size,
                                                         PROT_READ, MAP_PRIVATE, fd, 0);
        sys_close(fd);
        if (data == MAP_FAILED) {
            eprint("zcat: "); eprint(argv[i]); eprint(": cannot read\n");
            continue;
        }

        gzip_header_t hdr;
        if (gzip_parse_header(data, (unsigned long)st.st_size, &hdr) < 0) {
            eprint("zcat: "); eprint(argv[i]); eprint(": not in gzip format\n");
            sys_munmap(data, (size_t)st.st_size);
            continue;
        }

        long result = inflate_to_fd(data + hdr.data_offset, hdr.data_len, 1);
        sys_munmap(data, (size_t)st.st_size);

        if (result < 0) {
            eprint("zcat: "); eprint(argv[i]); eprint(": decompression error\n");
        }
        nfiles++;
    }

    if (nfiles == 0) {
        /* stdin mode */
        unsigned char buf[1024 * 1024];
        unsigned long total = 0;
        ssize_t r;
        while ((r = sys_read(0, buf + total, sizeof(buf) - total)) > 0)
            total += (unsigned long)r;

        if (total < 18) { eprint("zcat: stdin: not in gzip format\n"); return 1; }

        gzip_header_t hdr;
        if (gzip_parse_header(buf, total, &hdr) < 0) {
            eprint("zcat: stdin: not in gzip format\n");
            return 1;
        }

        if (inflate_to_fd(buf + hdr.data_offset, hdr.data_len, 1) < 0) {
            eprint("zcat: decompression error\n");
            return 1;
        }
    }

    return 0;
}

QEMT_ENTRY(zcat_main)

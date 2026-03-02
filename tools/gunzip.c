/*
 * gunzip - decompress .gz files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/deflate.h"
#include "../include/io.h"

int gunzip_main(int argc, char **argv, char **envp) {
    (void)envp;
    int keep = 0, to_stdout = 0, force = 0;
    const char *files[64];
    int nfiles = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == '-') {
                if (str_cmp(argv[i], "--keep") == 0) keep = 1;
                else if (str_cmp(argv[i], "--stdout") == 0) to_stdout = 1;
                else if (str_cmp(argv[i], "--force") == 0) force = 1;
                else if (str_cmp(argv[i], "--help") == 0) goto usage;
                continue;
            }
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                case 'k': keep = 1; break;
                case 'c': to_stdout = 1; break;
                case 'f': force = 1; break;
                case 'h': goto usage;
                default: break;
                }
            }
        } else {
            if (nfiles < 64) files[nfiles++] = argv[i];
        }
    }

    if (nfiles == 0) {
        /* Read from stdin, write to stdout */
        unsigned char buf[1024 * 1024]; /* 1MB read buffer */
        unsigned long total = 0;
        ssize_t r;
        while ((r = sys_read(0, buf + total, sizeof(buf) - total)) > 0)
            total += (unsigned long)r;

        if (total < 18) { eprint("gunzip: not in gzip format\n"); return 1; }

        gzip_header_t hdr;
        if (gzip_parse_header(buf, total, &hdr) < 0) {
            eprint("gunzip: not in gzip format\n");
            return 1;
        }

        long result = inflate_to_fd(buf + hdr.data_offset, hdr.data_len, 1);
        if (result < 0) { eprint("gunzip: decompression error\n"); return 1; }
        return 0;
    }

    for (int f = 0; f < nfiles; f++) {
        const char *path = files[f];

        /* Verify .gz extension */
        if (!force && !ends_with(path, ".gz")) {
            eprint("gunzip: "); eprint(path);
            eprint(": unknown suffix -- ignored\n");
            continue;
        }

        int fd = sys_open(path, O_RDONLY, 0);
        if (fd < 0) {
            eprint("gunzip: "); eprint(path); eprint(": No such file\n");
            continue;
        }

        /* Get file size and mmap it */
        struct stat st;
        if (sys_fstat(fd, &st) < 0 || st.st_size < 18) {
            eprint("gunzip: "); eprint(path); eprint(": not in gzip format\n");
            sys_close(fd);
            continue;
        }

        unsigned char *data = (unsigned char *)sys_mmap(NULL, (size_t)st.st_size,
                                                         PROT_READ, MAP_PRIVATE, fd, 0);
        sys_close(fd);
        if (data == MAP_FAILED) {
            eprint("gunzip: "); eprint(path); eprint(": cannot read file\n");
            continue;
        }

        gzip_header_t hdr;
        if (gzip_parse_header(data, (unsigned long)st.st_size, &hdr) < 0) {
            eprint("gunzip: "); eprint(path); eprint(": not in gzip format\n");
            sys_munmap(data, (size_t)st.st_size);
            continue;
        }

        int out_fd;
        char outpath[PATH_MAX];

        if (to_stdout) {
            out_fd = 1;
        } else {
            /* Remove .gz extension for output name */
            str_cpy(outpath, path);
            size_t len = str_len(outpath);
            if (len > 3 && str_cmp(outpath + len - 3, ".gz") == 0)
                outpath[len - 3] = '\0';
            else
                str_cat(outpath, ".out");

            out_fd = sys_open(outpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) {
                eprint("gunzip: cannot create output file\n");
                sys_munmap(data, (size_t)st.st_size);
                continue;
            }
        }

        long result = inflate_to_fd(data + hdr.data_offset, hdr.data_len, out_fd);

        if (!to_stdout) sys_close(out_fd);
        sys_munmap(data, (size_t)st.st_size);

        if (result < 0) {
            eprint("gunzip: "); eprint(path); eprint(": decompression error\n");
            if (!to_stdout) sys_unlink(outpath);
            continue;
        }

        /* Remove original file unless -k */
        if (!to_stdout && !keep)
            sys_unlink(path);
    }

    return 0;

usage:
    print("Usage: gunzip [-ckf] [file.gz ...]\n");
    print("  -c  Write to stdout\n");
    print("  -k  Keep original file\n");
    print("  -f  Force (process files without .gz suffix)\n");
    print("\nWith no files, reads from stdin, writes to stdout.\n");
    return 0;
}

QEMT_ENTRY(gunzip_main)

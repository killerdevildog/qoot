/*
 * dd - block copy / convert (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static unsigned long dd_parse_size(const char *s) {
    unsigned long val = 0;
    while (*s >= '0' && *s <= '9') { val = val * 10 + (unsigned long)(*s - '0'); s++; }
    /* Suffixes */
    if (*s == 'k' || *s == 'K') val *= 1024;
    else if (*s == 'M') val *= 1024 * 1024;
    else if (*s == 'G') val *= 1024UL * 1024 * 1024;
    else if (*s == 'b') val *= 512;
    return val;
}

int dd_main(int argc, char **argv, char **envp) {
    (void)envp;
    const char *if_path = NULL, *of_path = NULL;
    unsigned long bs = 512, count = 0, skip = 0, seek_off = 0;
    int count_set = 0;

    for (int i = 1; i < argc; i++) {
        if (starts_with(argv[i], "if=")) if_path = argv[i] + 3;
        else if (starts_with(argv[i], "of=")) of_path = argv[i] + 3;
        else if (starts_with(argv[i], "bs=")) bs = dd_parse_size(argv[i] + 3);
        else if (starts_with(argv[i], "count=")) { count = dd_parse_size(argv[i] + 6); count_set = 1; }
        else if (starts_with(argv[i], "skip=")) skip = dd_parse_size(argv[i] + 5);
        else if (starts_with(argv[i], "seek=")) seek_off = dd_parse_size(argv[i] + 5);
        else if (str_cmp(argv[i], "--help") == 0 || str_cmp(argv[i], "-h") == 0) {
            print("Usage: dd [if=FILE] [of=FILE] [bs=N] [count=N] [skip=N] [seek=N]\n");
            print("  if=FILE   Input file (default: stdin)\n");
            print("  of=FILE   Output file (default: stdout)\n");
            print("  bs=N      Block size (default: 512)\n");
            print("  count=N   Copy only N blocks\n");
            print("  skip=N    Skip N blocks at start of input\n");
            print("  seek=N    Skip N blocks at start of output\n");
            print("  Suffixes: k=1024, M=1048576, G=1073741824, b=512\n");
            return 0;
        }
    }

    if (bs == 0) { eprint("dd: invalid block size\n"); return 1; }
    if (bs > 64 * 1024 * 1024) { eprint("dd: block size too large\n"); return 1; }

    int in_fd = 0, out_fd = 1;

    if (if_path) {
        in_fd = sys_open(if_path, O_RDONLY, 0);
        if (in_fd < 0) { eprint("dd: cannot open '"); eprint(if_path); eprint("'\n"); return 1; }
    }

    if (of_path) {
        out_fd = sys_open(of_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) { eprint("dd: cannot open '"); eprint(of_path); eprint("'\n"); return 1; }
    }

    /* Skip input blocks */
    if (skip > 0) {
        sys_lseek(in_fd, (off_t)(skip * bs), SEEK_SET);
    }

    /* Seek output blocks */
    if (seek_off > 0) {
        sys_lseek(out_fd, (off_t)(seek_off * bs), SEEK_SET);
    }

    /* Allocate buffer on stack or use mmap for large blocks */
    unsigned char stack_buf[65536];
    unsigned char *buf = stack_buf;
    if (bs > sizeof(stack_buf)) {
        buf = (unsigned char *)sys_mmap(NULL, bs, PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (buf == MAP_FAILED) { eprint("dd: cannot allocate buffer\n"); return 1; }
    }

    unsigned long blocks_in = 0, blocks_out = 0, partial_in = 0, partial_out = 0;
    unsigned long total_bytes = 0;

    while (!count_set || blocks_in + partial_in < count) {
        ssize_t r = sys_read(in_fd, buf, bs);
        if (r <= 0) break;

        if ((unsigned long)r == bs) blocks_in++;
        else partial_in++;

        ssize_t w = 0;
        unsigned long written = 0;
        while (written < (unsigned long)r) {
            w = sys_write(out_fd, buf + written, (unsigned long)r - written);
            if (w <= 0) goto done;
            written += (unsigned long)w;
        }

        if (written == bs) blocks_out++;
        else partial_out++;
        total_bytes += written;
    }

done:
    /* Print summary to stderr */
    {
        char num[21];
        ulong_to_str(num, blocks_in); eprint(num); eprint("+");
        ulong_to_str(num, partial_in); eprint(num); eprint(" records in\n");
        ulong_to_str(num, blocks_out); eprint(num); eprint("+");
        ulong_to_str(num, partial_out); eprint(num); eprint(" records out\n");
        ulong_to_str(num, total_bytes); eprint(num); eprint(" bytes copied\n");
    }

    if (bs > sizeof(stack_buf)) sys_munmap(buf, bs);
    if (if_path) sys_close(in_fd);
    if (of_path) sys_close(out_fd);
    return 0;
}

QEMT_ENTRY(dd_main)

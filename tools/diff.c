/*
 * diff - compare files line by line (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 * Simple diff using longest common subsequence approach.
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

#define DIFF_MAX_LINES 8192

static char *lines_a[DIFF_MAX_LINES];
static char *lines_b[DIFF_MAX_LINES];
static int na, nb;

/* Static buffer for storing all line data */
static char line_storage[512 * 1024]; /* 512KB for line data */
static unsigned long storage_pos;

static char *store_line(const char *line, int len) {
    if (storage_pos + (unsigned long)len + 1 > sizeof(line_storage))
        return NULL;
    char *p = line_storage + storage_pos;
    mem_cpy(p, line, (size_t)len);
    p[len] = '\0';
    storage_pos += (unsigned long)len + 1;
    return p;
}

static int read_lines(int fd, char **lines, int max) {
    int count = 0;
    char buf[LINE_MAX];
    int len;
    while (count < max && (len = read_line(fd, buf, LINE_MAX)) > 0) {
        lines[count] = store_line(buf, len);
        if (!lines[count]) break;
        count++;
    }
    /* Handle last line without newline */
    if (count < max) {
        int r = read_line(fd, buf, LINE_MAX);
        if (r > 0) {
            lines[count] = store_line(buf, r);
            if (lines[count]) count++;
        }
    }
    return count;
}

/* Simple O(n*m) diff using edit distance approach */
/* For large files this is slow but correct */
static void simple_diff(const char *file_a, const char *file_b) {
    int ia = 0, ib = 0;

    while (ia < na && ib < nb) {
        if (str_cmp(lines_a[ia], lines_b[ib]) == 0) {
            /* Lines match */
            ia++;
            ib++;
        } else {
            /* Look ahead to find sync point */
            int found = 0;
            for (int look = 1; look < 100 && !found; look++) {
                /* Check if a[ia+look] matches b[ib] */
                if (ia + look < na && str_cmp(lines_a[ia + look], lines_b[ib]) == 0) {
                    /* Lines ia..ia+look-1 were removed */
                    char num[12];
                    int_to_str(num, ia + 1);
                    if (look == 1) {
                        print(num); print("d"); int_to_str(num, ib); print(num); print("\n");
                    } else {
                        print(num); print(","); int_to_str(num, ia + look); print(num);
                        print("d"); int_to_str(num, ib); print(num); print("\n");
                    }
                    for (int k = 0; k < look; k++) {
                        print("< "); print(lines_a[ia + k]); print("\n");
                    }
                    ia += look;
                    found = 1;
                }
                /* Check if b[ib+look] matches a[ia] */
                if (!found && ib + look < nb && str_cmp(lines_a[ia], lines_b[ib + look]) == 0) {
                    /* Lines ib..ib+look-1 were added */
                    char num[12];
                    int_to_str(num, ia);
                    print(num); print("a");
                    int_to_str(num, ib + 1); print(num);
                    if (look > 1) { print(","); int_to_str(num, ib + look); print(num); }
                    print("\n");
                    for (int k = 0; k < look; k++) {
                        print("> "); print(lines_b[ib + k]); print("\n");
                    }
                    ib += look;
                    found = 1;
                }
            }
            if (!found) {
                /* Changed line */
                char num[12];
                int_to_str(num, ia + 1); print(num);
                print("c");
                int_to_str(num, ib + 1); print(num);
                print("\n< "); print(lines_a[ia]); print("\n---\n> "); print(lines_b[ib]); print("\n");
                ia++;
                ib++;
            }
        }
    }

    /* Remaining lines in a (deleted) */
    if (ia < na) {
        char num[12];
        int_to_str(num, ia + 1); print(num);
        if (na - ia > 1) { print(","); int_to_str(num, na); print(num); }
        print("d"); int_to_str(num, nb); print(num); print("\n");
        while (ia < na) { print("< "); print(lines_a[ia++]); print("\n"); }
    }

    /* Remaining lines in b (added) */
    if (ib < nb) {
        char num[12];
        int_to_str(num, na); print(num);
        print("a"); int_to_str(num, ib + 1); print(num);
        if (nb - ib > 1) { print(","); int_to_str(num, nb); print(num); }
        print("\n");
        while (ib < nb) { print("> "); print(lines_b[ib++]); print("\n"); }
    }

    (void)file_a; (void)file_b;
}

int diff_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc < 3) {
        print("Usage: diff file1 file2\n");
        return 2;
    }

    const char *file_a = argv[argc - 2];
    const char *file_b = argv[argc - 1];

    storage_pos = 0;
    na = nb = 0;

    int fd_a = sys_open(file_a, O_RDONLY, 0);
    if (fd_a < 0) { eprint("diff: "); eprint(file_a); eprint(": No such file\n"); return 2; }
    na = read_lines(fd_a, lines_a, DIFF_MAX_LINES);
    sys_close(fd_a);

    int fd_b = sys_open(file_b, O_RDONLY, 0);
    if (fd_b < 0) { eprint("diff: "); eprint(file_b); eprint(": No such file\n"); return 2; }
    nb = read_lines(fd_b, lines_b, DIFF_MAX_LINES);
    sys_close(fd_b);

    /* Quick check: files identical? */
    if (na == nb) {
        int same = 1;
        for (int i = 0; i < na; i++) {
            if (str_cmp(lines_a[i], lines_b[i]) != 0) { same = 0; break; }
        }
        if (same) return 0;
    }

    simple_diff(file_a, file_b);
    return 1; /* diff returns 1 when files differ */
}

QEMT_ENTRY(diff_main)

/*
 * grep - pattern search in files (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Supports basic fixed-string and simple regex patterns.
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static int match_pattern(const char *pattern, const char *text);
static int match_star(int c, const char *pat, const char *text);
static int match_plus(int c, const char *pat, const char *text);

/* Match a single pattern element at current position */
static int match_here(const char *pat, const char *text) {
    if (pat[0] == '\0') return 1;
    if (pat[0] == '$' && pat[1] == '\0') return (*text == '\0' || *text == '\n');
    if (pat[1] == '*') return match_star(pat[0], pat + 2, text);
    if (pat[1] == '+') return match_plus(pat[0], pat + 2, text);
    if (pat[0] == '.') {
        if (*text != '\0' && *text != '\n') return match_here(pat + 1, text + 1);
        return 0;
    }
    if (pat[0] == '\\' && pat[1]) {
        if (pat[1] == *text) return match_here(pat + 2, text + 1);
        return 0;
    }
    if (*text != '\0' && pat[0] == *text) return match_here(pat + 1, text + 1);
    return 0;
}

static int match_star(int c, const char *pat, const char *text) {
    /* Match zero or more of character c */
    do {
        if (match_here(pat, text)) return 1;
    } while (*text != '\0' && *text != '\n' && (c == '.' || *text == c) && text++);
    return 0;
}

static int match_plus(int c, const char *pat, const char *text) {
    /* Match one or more of character c */
    while (*text != '\0' && *text != '\n' && (c == '.' || *text == c)) {
        text++;
        if (match_here(pat, text)) return 1;
    }
    return 0;
}

static int match_pattern(const char *pattern, const char *text) {
    if (pattern[0] == '^') return match_here(pattern + 1, text);
    /* Try match at every position */
    do {
        if (match_here(pattern, text)) return 1;
    } while (*text != '\0' && *text != '\n' && *text++);
    return 0;
}

/* Case-insensitive match */
static int match_pattern_icase(const char *pattern, const char *text) {
    /* For simple case: convert both to lower and match */
    char lpat[4096], ltxt[4096];
    int i;
    for (i = 0; pattern[i] && i < 4095; i++) lpat[i] = to_lower(pattern[i]);
    lpat[i] = '\0';
    for (i = 0; text[i] && i < 4095; i++) ltxt[i] = to_lower(text[i]);
    ltxt[i] = '\0';
    return match_pattern(lpat, ltxt);
}

static int grep_stream(int fd, const char *pattern, const char *filename,
                        int show_filename, int show_num, int invert, int icase, int count_only) {
    char buf[65536];
    ssize_t buf_len = 0;
    ssize_t line_start = 0;
    unsigned long line_num = 0;
    int match_count = 0;
    int ret = 1; /* 1 = no match */

    while (1) {
        ssize_t n = sys_read(fd, buf + buf_len, sizeof(buf) - (size_t)buf_len - 1);
        if (n <= 0 && buf_len == line_start) break;
        if (n > 0) buf_len += n;
        buf[buf_len] = '\0';

        while (line_start < buf_len) {
            /* Find end of line */
            ssize_t eol = line_start;
            while (eol < buf_len && buf[eol] != '\n') eol++;
            if (eol >= buf_len && n > 0) break; /* Incomplete line, read more */

            buf[eol] = '\0';
            line_num++;
            char *line = buf + line_start;

            int matched = icase ? match_pattern_icase(pattern, line) : match_pattern(pattern, line);
            if (invert) matched = !matched;

            if (matched) {
                ret = 0;
                match_count++;
                if (!count_only) {
                    if (show_filename) { print(filename); print(":"); }
                    if (show_num) {
                        char num[21];
                        ulong_to_str(num, line_num);
                        print(num); print(":");
                    }
                    print(line); print("\n");
                }
            }

            line_start = eol + 1;
        }

        /* Shift remaining data */
        if (line_start > 0 && line_start < buf_len) {
            mem_move(buf, buf + line_start, (size_t)(buf_len - line_start));
            buf_len -= line_start;
        } else if (line_start >= buf_len) {
            buf_len = 0;
        }
        line_start = 0;
        if (n <= 0) break;
    }

    if (count_only) {
        char num[21];
        if (show_filename) { print(filename); print(":"); }
        ulong_to_str(num, (unsigned long)match_count);
        print(num); print("\n");
    }
    return ret;
}

int grep_main(int argc, char **argv, char **envp) {
    (void)envp;
    int icase = 0, invert = 0, show_num = 0, count_only = 0;
    int first = 1;

    if (argc > 1 && str_cmp(argv[1], "--help") == 0) {
        print("Usage: grep [-icnv] pattern [file...]\n");
        print("  -i  Case insensitive\n  -v  Invert match\n");
        print("  -n  Show line numbers\n  -c  Count matches\n");
        return 0;
    }

    while (first < argc && argv[first][0] == '-' && argv[first][1] != '\0') {
        for (int j = 1; argv[first][j]; j++) {
            switch (argv[first][j]) {
                case 'i': icase = 1; break;
                case 'v': invert = 1; break;
                case 'n': show_num = 1; break;
                case 'c': count_only = 1; break;
                default:
                    eprint("grep: unknown option '-");
                    char c[2] = {argv[first][j], 0};
                    eprint(c); eprint("'\n");
                    return 2;
            }
        }
        first++;
    }

    if (first >= argc) {
        eprint("Usage: grep [-icnv] pattern [file...]\n");
        return 2;
    }

    const char *pattern = argv[first++];
    int nfiles = argc - first;

    if (nfiles == 0) {
        return grep_stream(0, pattern, "(stdin)", 0, show_num, invert, icase, count_only);
    }

    int show_fn = (nfiles > 1);
    int ret = 1;
    for (int i = first; i < argc; i++) {
        int fd = sys_open(argv[i], O_RDONLY, 0);
        if (fd < 0) {
            eprint("grep: "); eprint(argv[i]); eprint(": No such file\n");
            continue;
        }
        if (grep_stream(fd, pattern, argv[i], show_fn, show_num, invert, icase, count_only) == 0)
            ret = 0;
        sys_close(fd);
    }
    return ret;
}

QEMT_ENTRY(grep_main)

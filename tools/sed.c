/*
 * sed - stream editor (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 * Supports: s/old/new/[g], d, p, q, line addressing
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

#define SED_MAX_CMDS 32

struct sed_cmd {
    int line_start;   /* 0 = all lines, -1 = last line ($) */
    int line_end;     /* 0 = single line or all */
    char type;        /* 's', 'd', 'p', 'q' */
    char find[256];
    char replace[256];
    int global;       /* /g flag for substitution */
};

static int parse_sed_cmd(const char *expr, struct sed_cmd *cmd) {
    mem_set(cmd, 0, sizeof(*cmd));
    const char *p = expr;

    /* Parse address */
    if (*p >= '0' && *p <= '9') {
        cmd->line_start = (int)str_to_uint(p);
        while (*p >= '0' && *p <= '9') p++;
        if (*p == ',') {
            p++;
            if (*p == '$') { cmd->line_end = -1; p++; }
            else { cmd->line_end = (int)str_to_uint(p); while (*p >= '0' && *p <= '9') p++; }
        }
    } else if (*p == '$') {
        cmd->line_start = -1;
        p++;
    }

    cmd->type = *p++;

    if (cmd->type == 's') {
        if (*p == '\0') return -1;
        char delim = *p++;
        int i = 0;
        while (*p && *p != delim && i < 255) cmd->find[i++] = *p++;
        cmd->find[i] = '\0';
        if (*p == delim) p++;
        i = 0;
        while (*p && *p != delim && i < 255) cmd->replace[i++] = *p++;
        cmd->replace[i] = '\0';
        if (*p == delim) p++;
        if (*p == 'g') cmd->global = 1;
    }

    return 0;
}

static int matches_addr(const struct sed_cmd *cmd, int lineno, int is_last) {
    if (cmd->line_start == 0) return 1; /* no address = all lines */
    int start = (cmd->line_start == -1) ? (is_last ? lineno : -999) : cmd->line_start;
    if (cmd->line_end == 0) return lineno == start;
    int end = (cmd->line_end == -1) ? (is_last ? lineno : 999999) : cmd->line_end;
    return lineno >= start && lineno <= end;
}

static void sed_substitute(char *line, const char *find, const char *replace, int global) {
    char result[LINE_MAX];
    int rpos = 0;
    size_t flen = str_len(find);

    if (flen == 0) { print(line); print("\n"); return; }

    char *p = line;
    int did = 0;
    while (*p) {
        char *match = str_str(p, find);
        if (match && (!did || global)) {
            /* Copy text before match */
            while (p < match && rpos < LINE_MAX - 1) result[rpos++] = *p++;
            /* Copy replacement */
            const char *r = replace;
            while (*r && rpos < LINE_MAX - 1) result[rpos++] = *r++;
            p = match + flen;
            did = 1;
        } else {
            if (rpos < LINE_MAX - 1) result[rpos++] = *p;
            p++;
        }
    }
    result[rpos] = '\0';
    print(result);
    print("\n");
}

int sed_main(int argc, char **argv, char **envp) {
    (void)envp;
    struct sed_cmd cmds[SED_MAX_CMDS];
    int ncmds = 0;
    int suppress = 0;
    const char *file = NULL;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-n") == 0) {
            suppress = 1;
        } else if (str_cmp(argv[i], "-e") == 0 && i + 1 < argc) {
            if (ncmds < SED_MAX_CMDS)
                parse_sed_cmd(argv[++i], &cmds[ncmds++]);
        } else if (str_cmp(argv[i], "-h") == 0 || str_cmp(argv[i], "--help") == 0) {
            print("Usage: sed [-n] [-e CMD] 'CMD' [file]\n");
            print("Commands:\n");
            print("  s/old/new/[g]  Substitute\n");
            print("  d              Delete line\n");
            print("  p              Print line\n");
            print("  q              Quit\n");
            print("  [N] or [N,M]   Line address\n");
            return 0;
        } else if (argv[i][0] != '-' && ncmds == 0) {
            if (ncmds < SED_MAX_CMDS)
                parse_sed_cmd(argv[i], &cmds[ncmds++]);
        } else if (argv[i][0] != '-') {
            file = argv[i];
        }
    }

    if (ncmds == 0) {
        eprint("sed: no commands specified\n");
        return 1;
    }

    int fd = 0;
    if (file) {
        fd = sys_open(file, O_RDONLY, 0);
        if (fd < 0) { eprint("sed: cannot open '"); eprint(file); eprint("'\n"); return 1; }
    }

    /* Read all lines to know last line for $ addressing */
    char line[LINE_MAX];
    int lineno = 0;
    int len;

    while ((len = read_line(fd, line, LINE_MAX)) > 0) {
        lineno++;
        int deleted = 0;
        int printed = 0;

        for (int c = 0; c < ncmds; c++) {
            /* Peek ahead to check if this might be last line */
            /* (simplified: we don't precompute, $ only works for quit) */
            if (!matches_addr(&cmds[c], lineno, 0)) continue;

            switch (cmds[c].type) {
            case 'd':
                deleted = 1;
                break;
            case 'p':
                print(line); print("\n");
                printed = 1;
                break;
            case 'q':
                if (!suppress) { print(line); print("\n"); }
                goto done;
            case 's':
                sed_substitute(line, cmds[c].find, cmds[c].replace, cmds[c].global);
                printed = 1;
                break;
            }
            if (deleted) break;
        }

        if (!deleted && !suppress && !printed) {
            print(line); print("\n");
        }
    }

done:
    if (file) sys_close(fd);
    return 0;
}

QEMT_ENTRY(sed_main)

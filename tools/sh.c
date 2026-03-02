/*
 * sh - minimal emergency shell (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Features: command execution, pipes (|), redirects (>, <, >>),
 * cd, exit, export, basic variable expansion ($VAR), PATH lookup.
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

#define MAX_ARGS 64
#define MAX_LINE 4096
#define MAX_ENV  256

static char *env_table[MAX_ENV];
static int env_count = 0;

static void init_env(char **envp) {
    for (env_count = 0; envp[env_count] && env_count < MAX_ENV - 1; env_count++)
        env_table[env_count] = envp[env_count];
    env_table[env_count] = NULL;
}

static char *get_env(const char *name) {
    size_t nlen = str_len(name);
    for (int i = 0; i < env_count; i++) {
        if (str_ncmp(env_table[i], name, nlen) == 0 && env_table[i][nlen] == '=')
            return env_table[i] + nlen + 1;
    }
    return NULL;
}

static void set_env(const char *entry) {
    /* entry is "KEY=VALUE" */
    const char *eq = str_chr(entry, '=');
    if (!eq) return;
    size_t klen = (size_t)(eq - entry);

    /* Find existing */
    for (int i = 0; i < env_count; i++) {
        if (str_ncmp(env_table[i], entry, klen) == 0 && env_table[i][klen] == '=') {
            env_table[i] = (char *)entry;
            return;
        }
    }
    if (env_count < MAX_ENV - 1) {
        env_table[env_count++] = (char *)entry;
        env_table[env_count] = NULL;
    }
}

/* Expand $VAR references in a string */
static void expand_vars(const char *src, char *dst, size_t dst_sz) {
    size_t di = 0;
    for (size_t si = 0; src[si] && di < dst_sz - 1; ) {
        if (src[si] == '$' && src[si+1]) {
            si++;
            char varname[256];
            int vi = 0;
            if (src[si] == '{') {
                si++;
                while (src[si] && src[si] != '}' && vi < 255) varname[vi++] = src[si++];
                if (src[si] == '}') si++;
            } else if (src[si] == '?') {
                /* $? = last exit status - simplified */
                varname[0] = '?'; vi = 1; si++;
            } else {
                while ((is_alnum(src[si]) || src[si] == '_') && vi < 255) varname[vi++] = src[si++];
            }
            varname[vi] = '\0';
            char *val = get_env(varname);
            if (val) {
                while (*val && di < dst_sz - 1) dst[di++] = *val++;
            }
        } else {
            dst[di++] = src[si++];
        }
    }
    dst[di] = '\0';
}

static int find_in_path(const char *cmd, char *out, size_t out_sz) {
    if (str_chr(cmd, '/')) {
        if (sys_access(cmd, 1) == 0) { str_ncpy(out, cmd, out_sz); return 0; }
        return -1;
    }
    char *path = get_env("PATH");
    if (!path) path = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";

    char pcopy[4096];
    str_ncpy(pcopy, path, sizeof(pcopy) - 1);
    pcopy[sizeof(pcopy) - 1] = '\0';

    char *p = pcopy;
    while (*p) {
        char *colon = str_chr(p, ':');
        if (colon) *colon = '\0';
        str_ncpy(out, p, out_sz - 1);
        str_cat(out, "/");
        str_cat(out, cmd);
        if (sys_access(out, 1) == 0) return 0;
        if (colon) p = colon + 1; else break;
    }
    return -1;
}

static int parse_args(char *line, char **args) {
    int count = 0;
    char *p = line;

    while (*p && count < MAX_ARGS - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (*p == '"') {
            p++;
            args[count++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = '\0';
        } else if (*p == '\'') {
            p++;
            args[count++] = p;
            while (*p && *p != '\'') p++;
            if (*p == '\'') *p++ = '\0';
        } else {
            args[count++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
        }
    }
    args[count] = NULL;
    return count;
}

static int execute_cmd(char **args, int argc) {
    if (argc == 0) return 0;

    /* Builtins */
    if (str_cmp(args[0], "cd") == 0) {
        const char *dir = argc > 1 ? args[1] : get_env("HOME");
        if (!dir) dir = "/";
        if (sys_chdir(dir) < 0) {
            eprint("sh: cd: "); eprint(dir); eprint(": No such directory\n");
            return 1;
        }
        return 0;
    }

    if (str_cmp(args[0], "exit") == 0) {
        int code = argc > 1 ? (int)str_to_uint(args[1]) : 0;
        sys_exit(code);
        return 0; /* unreachable */
    }

    if (str_cmp(args[0], "export") == 0) {
        for (int i = 1; i < argc; i++) {
            if (str_chr(args[i], '=')) {
                /* Need persistent storage */
                static char env_storage[64][256];
                static int env_idx = 0;
                if (env_idx < 64) {
                    str_ncpy(env_storage[env_idx], args[i], 255);
                    set_env(env_storage[env_idx]);
                    env_idx++;
                }
            }
        }
        return 0;
    }

    if (str_cmp(args[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        if (sys_getcwd(cwd, sizeof(cwd)) >= 0) { print(cwd); print("\n"); }
        return 0;
    }

    /* External command */
    char path[PATH_MAX];
    if (find_in_path(args[0], path, sizeof(path)) < 0) {
        eprint("sh: "); eprint(args[0]); eprint(": command not found\n");
        return 127;
    }

    int pid = sys_fork();
    if (pid < 0) { eprint("sh: fork failed\n"); return 1; }

    if (pid == 0) {
        /* Child */
        sys_execve(path, args, env_table);
        eprint("sh: exec failed: "); eprint(path); eprint("\n");
        sys_exit(127);
    }

    /* Parent: wait */
    int status = 0;
    sys_wait4(pid, &status, 0, NULL);
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 1;
}

/* Execute a command line with pipe and redirect support */
static int execute_line(char *line) {
    /* Handle pipes: split on unquoted | */
    char *segments[16];
    int nseg = 0;

    char *p = line;
    segments[nseg++] = p;
    int in_sq = 0, in_dq = 0;
    while (*p) {
        if (*p == '\'' && !in_dq) in_sq = !in_sq;
        else if (*p == '"' && !in_sq) in_dq = !in_dq;
        else if (*p == '|' && !in_sq && !in_dq) {
            *p = '\0';
            if (nseg < 16) segments[nseg++] = p + 1;
        }
        p++;
    }

    if (nseg == 1) {
        /* Handle redirects */
        char *args[MAX_ARGS];
        char *infile = NULL, *outfile = NULL;
        int append = 0;

        char expanded[MAX_LINE];
        expand_vars(segments[0], expanded, sizeof(expanded));

        int argc = parse_args(expanded, args);

        /* Scan for redirects */
        int new_argc = 0;
        for (int i = 0; i < argc; i++) {
            if (str_cmp(args[i], ">") == 0 && i + 1 < argc) { outfile = args[++i]; append = 0; }
            else if (str_cmp(args[i], ">>") == 0 && i + 1 < argc) { outfile = args[++i]; append = 1; }
            else if (str_cmp(args[i], "<") == 0 && i + 1 < argc) { infile = args[++i]; }
            else args[new_argc++] = args[i];
        }
        args[new_argc] = NULL;

        if (infile || outfile) {
            int pid = sys_fork();
            if (pid == 0) {
                if (infile) {
                    int fd = sys_open(infile, O_RDONLY, 0);
                    if (fd < 0) { eprint("sh: cannot open "); eprint(infile); eprint("\n"); sys_exit(1); }
                    sys_dup2(fd, 0); sys_close(fd);
                }
                if (outfile) {
                    int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
                    int fd = sys_open(outfile, flags, 0644);
                    if (fd < 0) { eprint("sh: cannot open "); eprint(outfile); eprint("\n"); sys_exit(1); }
                    sys_dup2(fd, 1); sys_close(fd);
                }

                /* Builtins in redirect context: just fork+exec, but handle builtins too */
                char path[PATH_MAX];
                if (find_in_path(args[0], path, sizeof(path)) < 0) {
                    eprint("sh: "); eprint(args[0]); eprint(": command not found\n");
                    sys_exit(127);
                }
                sys_execve(path, args, env_table);
                sys_exit(127);
            }
            int status = 0;
            sys_wait4(pid, &status, 0, NULL);
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }

        return execute_cmd(args, new_argc);
    }

    /* Pipeline execution */
    int prev_fd = -1;
    for (int s = 0; s < nseg; s++) {
        int pipefd[2] = {-1, -1};
        if (s < nseg - 1) sys_pipe(pipefd);

        char expanded[MAX_LINE];
        expand_vars(segments[s], expanded, sizeof(expanded));

        char *args[MAX_ARGS];
        int argc = parse_args(expanded, args);
        (void)argc;

        char path[PATH_MAX];
        if (find_in_path(args[0], path, sizeof(path)) < 0) {
            eprint("sh: "); eprint(args[0]); eprint(": command not found\n");
            if (prev_fd >= 0) sys_close(prev_fd);
            if (pipefd[0] >= 0) { sys_close(pipefd[0]); sys_close(pipefd[1]); }
            return 127;
        }

        int pid = sys_fork();
        if (pid == 0) {
            if (prev_fd >= 0) { sys_dup2(prev_fd, 0); sys_close(prev_fd); }
            if (pipefd[1] >= 0) { sys_close(pipefd[0]); sys_dup2(pipefd[1], 1); sys_close(pipefd[1]); }
            sys_execve(path, args, env_table);
            sys_exit(127);
        }

        if (prev_fd >= 0) sys_close(prev_fd);
        if (pipefd[1] >= 0) sys_close(pipefd[1]);
        prev_fd = pipefd[0];

        if (s == nseg - 1) {
            int status = 0;
            sys_wait4(pid, &status, 0, NULL);
            /* Wait for all remaining children */
            while (sys_wait4(-1, &status, 0, NULL) > 0);
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    }
    return 0;
}

int sh_main(int argc, char **argv, char **envp) {
    init_env(envp);

    /* Execute a script file with -c */
    if (argc > 2 && str_cmp(argv[1], "-c") == 0) {
        char line[MAX_LINE];
        str_ncpy(line, argv[2], sizeof(line) - 1);
        return execute_line(line);
    }

    /* Execute a script file */
    if (argc > 1 && argv[1][0] != '-') {
        int fd = sys_open(argv[1], O_RDONLY, 0);
        if (fd < 0) {
            eprint("sh: cannot open '"); eprint(argv[1]); eprint("'\n");
            return 1;
        }
        char buf[65536];
        ssize_t n = sys_read(fd, buf, sizeof(buf) - 1);
        sys_close(fd);
        if (n <= 0) return 0;
        buf[n] = '\0';

        int ret = 0;
        char *line = buf;
        while (*line) {
            char *nl = str_chr(line, '\n');
            if (nl) *nl = '\0';
            /* Skip comments and empty lines */
            char *trimmed = line;
            while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
            if (*trimmed && *trimmed != '#') {
                char cmd_line[MAX_LINE];
                str_ncpy(cmd_line, trimmed, sizeof(cmd_line) - 1);
                ret = execute_line(cmd_line);
            }
            if (nl) line = nl + 1; else break;
        }
        return ret;
    }

    /* Interactive mode */
    int interactive = 1;
    /* Check if stdin is a terminal */
    struct termios t;
    if (sys_ioctl(0, TCGETS, &t) < 0) interactive = 0;

    int last_status = 0;
    (void)last_status;

    while (1) {
        if (interactive) {
            char cwd[PATH_MAX];
            if (sys_getcwd(cwd, sizeof(cwd)) >= 0) {
                print("\033[1;34m"); print(cwd); print("\033[0m");
            }
            print(" \033[1;32mqemt$\033[0m ");
        }

        char line[MAX_LINE];
        ssize_t n = read_line(0, line, sizeof(line));
        if (n <= 0) break;

        /* Strip trailing newline */
        while (n > 0 && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';
        if (n == 0) continue;

        /* Skip comments */
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (*trimmed == '#' || *trimmed == '\0') continue;

        char cmd_line[MAX_LINE];
        str_ncpy(cmd_line, trimmed, sizeof(cmd_line) - 1);
        last_status = execute_line(cmd_line);
    }

    if (interactive) print("exit\n");
    return 0;
}

QEMT_ENTRY(sh_main)

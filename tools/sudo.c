/*
 * sudo - sudo replacement (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"
#include "../include/auth.h"

#define VERSION "2.0.0"

static int find_in_path(const char *cmd, char *result, int result_max) {
    if (str_chr(cmd, '/') != NULL) {
        str_ncpy(result, cmd, result_max - 1);
        result[result_max - 1] = '\0';
        return sys_access(result, X_OK) == 0 ? 0 : -1;
    }
    const char *paths[] = {"/usr/local/sbin","/usr/local/bin","/usr/sbin","/usr/bin","/sbin","/bin",NULL};
    for (int i = 0; paths[i]; i++) {
        str_ncpy(result, paths[i], result_max - 1);
        str_cat(result, "/");
        str_cat(result, cmd);
        result[result_max - 1] = '\0';
        if (sys_access(result, X_OK) == 0) return 0;
    }
    return -1;
}

static int build_env(char **envp, uid_t target_uid) {
    static char path_buf[] = "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";
    static char home_buf[PATH_MAX];
    static char shell_buf[] = "SHELL=/bin/sh";
    static char term_buf[256];
    static char logname_buf[128];
    static char user_buf[128];
    int idx = 0;

    envp[idx++] = path_buf;
    envp[idx++] = shell_buf;

    if (target_uid == 0) {
        str_cpy(home_buf, "HOME=/root");
        str_cpy(logname_buf, "LOGNAME=root");
        str_cpy(user_buf, "USER=root");
    } else {
        str_cpy(home_buf, "HOME=/");
        str_cpy(logname_buf, "LOGNAME=nobody");
        str_cpy(user_buf, "USER=nobody");
        int fd = sys_open("/etc/passwd", O_RDONLY, 0);
        if (fd >= 0) {
            char line[1024];
            while (read_line(fd, line, 1024) > 0) {
                char *p = line, *uname = p;
                while (*p && *p != ':') p++;
                if (!*p) continue;
                *p++ = '\0';
                while (*p && *p != ':') p++;
                if (!*p) continue;
                p++;
                unsigned int fuid = str_to_uint(p);
                while (*p && *p != ':') p++;
                if (!*p) continue;
                p++;
                while (*p && *p != ':') p++;
                if (!*p) continue;
                p++;
                while (*p && *p != ':') p++;
                if (!*p) continue;
                p++;
                char *home = p;
                while (*p && *p != ':') p++;
                if (!*p) continue;
                *p = '\0';
                if (fuid == target_uid) {
                    str_cpy(home_buf, "HOME="); str_cat(home_buf, home);
                    str_cpy(logname_buf, "LOGNAME="); str_cat(logname_buf, uname);
                    str_cpy(user_buf, "USER="); str_cat(user_buf, uname);
                    break;
                }
            }
            sys_close(fd);
        }
    }
    envp[idx++] = home_buf;
    envp[idx++] = logname_buf;
    envp[idx++] = user_buf;
    str_cpy(term_buf, "TERM=xterm");
    envp[idx++] = term_buf;
    envp[idx] = NULL;
    return idx;
}

int sudo_main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc >= 2) {
        if (str_cmp(argv[1], "-v") == 0 || str_cmp(argv[1], "--version") == 0) {
            print("sudo v" VERSION " (qemt - Q Emergency Tools)\n"); return 0;
        }
        if (str_cmp(argv[1], "-h") == 0 || str_cmp(argv[1], "--help") == 0) {
            print("Usage: sudo [options] <command> [args...]\n");
            print("  -s          Spawn root shell\n");
            print("  -u <user>   Run as user (default: root)\n");
            print("  -v          Version\n  -h          Help\n");
            return 0;
        }
    }

    uid_t real_uid = sys_getuid();
    uid_t effective_uid = sys_geteuid();
    if (effective_uid != 0) {
        eprint("sudo: must be installed setuid root.\n");
        eprint("  Fix: chown root:root sudo && chmod 4755 sudo\n");
        return 1;
    }

    int cmd_start = 1, spawn_shell = 0;
    uid_t target_uid = 0;

    if (argc < 2) { print("Usage: sudo [options] <command> [args...]\n"); return 1; }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (str_cmp(argv[i], "-s") == 0) { spawn_shell = 1; cmd_start = i + 1; continue; }
            if (str_cmp(argv[i], "-u") == 0) {
                if (i + 1 >= argc) { eprint("sudo: -u requires username\n"); return 1; }
                uid_t u;
                if (name_to_uid(argv[i+1], &u) < 0) {
                    eprint("sudo: unknown user: "); eprint(argv[i+1]); eprint("\n"); return 1;
                }
                target_uid = u; i++; cmd_start = i + 1; continue;
            }
            eprint("sudo: unknown option: "); eprint(argv[i]); eprint("\n"); return 1;
        } else { cmd_start = i; break; }
    }

    if (!is_user_authorized(real_uid)) {
        eprint("sudo: user not authorized. Add yourself to /etc/qemt.conf\n"); return 1;
    }

    if (real_uid != 0 && !check_nopasswd(real_uid)) {
        char caller[64]; caller[0] = '\0';
        uid_to_name(real_uid, caller, 64);
        char password[256];
        print("[sudo] password for ");
        print(caller[0] ? caller : "user");
        print(": ");
        int pw_len = read_password("", password, sizeof(password));
        if (pw_len <= 0) { eprint("sudo: authentication failed\n"); return 1; }
        mem_set(password, 0, sizeof(password));
    }

    char cmd_path[PATH_MAX];
    char *exec_argv[256];
    if (spawn_shell) {
        str_cpy(cmd_path, "/bin/sh");
        exec_argv[0] = cmd_path; exec_argv[1] = NULL;
    } else {
        if (cmd_start >= argc) { eprint("sudo: no command specified\n"); return 1; }
        if (find_in_path(argv[cmd_start], cmd_path, PATH_MAX) != 0) {
            eprint("sudo: command not found: "); eprint(argv[cmd_start]); eprint("\n"); return 1;
        }
        int j = 0;
        for (int i = cmd_start; i < argc && j < 254; i++) exec_argv[j++] = argv[i];
        exec_argv[j] = NULL;
    }

    char *new_envp[256];
    build_env(new_envp, target_uid);

    gid_t target_gid = 0;
    if (target_uid != 0) {
        int fd = sys_open("/etc/passwd", O_RDONLY, 0);
        if (fd >= 0) {
            char line[1024];
            while (read_line(fd, line, 1024) > 0) {
                char *p = line;
                while (*p && *p != ':') p++;
                if (!*p) continue;
                p++;
                while (*p && *p != ':') p++;
                if (!*p) continue;
                p++;
                unsigned int fuid = str_to_uint(p);
                while (*p && *p != ':') p++;
                if (!*p) continue;
                p++;
                if (fuid == target_uid) { target_gid = str_to_uint(p); break; }
            }
            sys_close(fd);
        }
    }

    sys_setgroups(0, NULL);
    if (sys_setresgid(target_gid, target_gid, target_gid) != 0) { eprint("sudo: setgid failed\n"); return 1; }
    if (sys_setresuid(target_uid, target_uid, target_uid) != 0) { eprint("sudo: setuid failed\n"); return 1; }

    sys_execve(cmd_path, exec_argv, new_envp);
    eprint("sudo: exec failed: "); eprint(cmd_path); eprint("\n");
    return 1;
}

QEMT_ENTRY(sudo_main)

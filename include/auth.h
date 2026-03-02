/*
 * qemt - Authentication module (no glibc dependency)
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ---
 *
 * Handles user authorization for the sudo tool
 * by reading /etc/qemt.conf directly via raw syscalls.
 */

#ifndef QEMT_AUTH_H
#define QEMT_AUTH_H

#include "io.h"

#define MAX_LINE 1024
#define MAX_USERNAME 64

/* Check if user is authorized in /etc/qemt.conf */
static int is_user_authorized(uid_t uid) {
    if (uid == 0) return 1;

    int fd = sys_open("/etc/qemt.conf", O_RDONLY, 0);
    if (fd < 0) {
        eprint("sudo: /etc/qemt.conf not found. Only root can use sudo.\n");
        return 0;
    }

    char username[MAX_USERNAME];
    username[0] = '\0';
    uid_to_name(uid, username, MAX_USERNAME);

    if (username[0] == '\0') {
        sys_close(fd);
        eprint("sudo: could not determine username for uid\n");
        return 0;
    }

    char line[MAX_LINE];
    int authorized = 0;
    while (read_line(fd, line, MAX_LINE) > 0) {
        const char *l = skip_space(line);
        if (*l == '#' || *l == '\0') continue;

        char trimmed[MAX_LINE];
        str_ncpy(trimmed, l, MAX_LINE - 1);
        int len = (int)str_len(trimmed);
        while (len > 0 && is_space(trimmed[len - 1])) trimmed[--len] = '\0';

        if (str_cmp(trimmed, "ALL") == 0 || starts_with(trimmed, "ALL ")) {
            authorized = 1;
            break;
        }
        if (str_cmp(trimmed, username) == 0) {
            authorized = 1;
            break;
        }
        if (starts_with(trimmed, username)) {
            size_t ulen = str_len(username);
            if (trimmed[ulen] == '\0' || trimmed[ulen] == ' ' || trimmed[ulen] == '\t') {
                authorized = 1;
                break;
            }
        }
    }

    sys_close(fd);
    return authorized;
}

/* Check if user has NOPASSWD in config */
static int check_nopasswd(uid_t uid) {
    char username[MAX_USERNAME];
    username[0] = '\0';
    uid_to_name(uid, username, MAX_USERNAME);
    if (username[0] == '\0') return 0;

    int fd = sys_open("/etc/qemt.conf", O_RDONLY, 0);
    if (fd < 0) return 0;

    char line[MAX_LINE];
    int nopasswd = 0;
    while (read_line(fd, line, MAX_LINE) > 0) {
        const char *l = skip_space(line);
        if (*l == '#' || *l == '\0') continue;

        if (starts_with(l, username)) {
            const char *after = l + str_len(username);
            if (is_space(*after)) {
                after = skip_space(after);
                if (starts_with(after, "NOPASSWD")) { nopasswd = 1; break; }
            }
        }
        if (starts_with(l, "ALL")) {
            const char *after = l + 3;
            if (is_space(*after)) {
                after = skip_space(after);
                if (starts_with(after, "NOPASSWD")) { nopasswd = 1; break; }
            }
        }
    }

    sys_close(fd);
    return nopasswd;
}

#endif /* QEMT_AUTH_H */

/*
 * qemt - I/O helper functions (no glibc dependency)
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
 * Common I/O operations shared across all qemt tools.
 */

#ifndef QEMT_IO_H
#define QEMT_IO_H

#include "syscalls.h"
#include "string.h"

/* Write string to fd */
static inline void write_str(int fd, const char *s) {
    sys_write(fd, s, str_len(s));
}

/* Write full buffer to fd (handles partial writes) */
static inline int write_all(int fd, const void *buf, size_t len) {
    const unsigned char *p = (const unsigned char *)buf;
    while (len > 0) {
        ssize_t w = sys_write(fd, p, len);
        if (w <= 0) return -1;
        p += (size_t)w;
        len -= (size_t)w;
    }
    return 0;
}

/* Write to stdout / stderr */
static inline void print(const char *s) { write_str(1, s); }
static inline void eprint(const char *s) { write_str(2, s); }

/* Print a number */
static inline void print_uint(unsigned int n) {
    char buf[12]; uint_to_str(buf, n); print(buf);
}

static inline void print_int(int n) {
    char buf[12]; int_to_str(buf, n); print(buf);
}

static inline void print_ulong(unsigned long n) {
    char buf[21]; ulong_to_str(buf, n); print(buf);
}

static inline void print_long(long n) {
    char buf[21]; long_to_str(buf, n); print(buf);
}

/* Read a line from fd into buf. Returns bytes read, 0 on EOF, -1 on error. */
static inline int read_line(int fd, char *buf, int max) {
    int i = 0;
    char c;
    while (i < max - 1) {
        ssize_t r = sys_read(fd, &c, 1);
        if (r <= 0) { if (i == 0) return (int)r; break; }
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

/* Read password with echo disabled */
static inline int read_password(const char *prompt, char *buf, int max) {
    int tty_fd = sys_open("/dev/tty", O_RDWR, 0);
    if (tty_fd < 0) { eprint("cannot open terminal\n"); return -1; }

    struct termios old_term, new_term;
    sys_ioctl(tty_fd, TCGETS, &old_term);
    mem_cpy(&new_term, &old_term, sizeof(struct termios));
    new_term.c_lflag &= ~ECHO;
    sys_ioctl(tty_fd, TCSETSW, &new_term);

    if (prompt[0]) sys_write(tty_fd, prompt, str_len(prompt));

    int i = 0;
    char c;
    while (i < max - 1) {
        ssize_t r = sys_read(tty_fd, &c, 1);
        if (r <= 0) break;
        if (c == '\n' || c == '\r') break;
        buf[i++] = c;
    }
    buf[i] = '\0';

    sys_ioctl(tty_fd, TCSETSW, &old_term);
    sys_write(tty_fd, "\n", 1);
    sys_close(tty_fd);
    return i;
}

/* Lookup username from UID via /etc/passwd. Returns 1 if found. */
static inline int uid_to_name(uid_t uid, char *name, int name_max) {
    int fd = sys_open("/etc/passwd", O_RDONLY, 0);
    if (fd < 0) return 0;
    char line[1024];
    while (read_line(fd, line, 1024) > 0) {
        char *p = line, *uname = p;
        while (*p && *p != ':') p++;
        if (*p != ':') continue;
        *p++ = '\0';
        while (*p && *p != ':') p++;
        if (*p != ':') continue;
        p++;
        unsigned int fuid = str_to_uint(p);
        if (fuid == uid) {
            str_ncpy(name, uname, name_max - 1);
            name[name_max - 1] = '\0';
            sys_close(fd);
            return 1;
        }
    }
    sys_close(fd);
    return 0;
}

/* Lookup group name from GID via /etc/group. Returns 1 if found. */
static inline int gid_to_name(gid_t gid, char *name, int name_max) {
    int fd = sys_open("/etc/group", O_RDONLY, 0);
    if (fd < 0) return 0;
    char line[1024];
    while (read_line(fd, line, 1024) > 0) {
        char *p = line, *gname = p;
        while (*p && *p != ':') p++;
        if (*p != ':') continue;
        *p++ = '\0';
        while (*p && *p != ':') p++;
        if (*p != ':') continue;
        p++;
        unsigned int fgid = str_to_uint(p);
        if (fgid == gid) {
            str_ncpy(name, gname, name_max - 1);
            name[name_max - 1] = '\0';
            sys_close(fd);
            return 1;
        }
    }
    sys_close(fd);
    return 0;
}

/* Lookup UID from username via /etc/passwd. Returns -1 if not found. */
static inline int name_to_uid(const char *name, uid_t *uid) {
    int fd = sys_open("/etc/passwd", O_RDONLY, 0);
    if (fd < 0) return -1;
    char line[1024];
    while (read_line(fd, line, 1024) > 0) {
        char *p = line, *uname = p;
        while (*p && *p != ':') p++;
        if (*p != ':') continue;
        *p++ = '\0';
        while (*p && *p != ':') p++;
        if (*p != ':') continue;
        p++;
        if (str_cmp(uname, name) == 0) {
            *uid = str_to_uint(p);
            sys_close(fd);
            return 0;
        }
    }
    sys_close(fd);
    return -1;
}

/* Lookup GID from group name. Returns -1 if not found. */
static inline int name_to_gid(const char *name, gid_t *gid) {
    int fd = sys_open("/etc/group", O_RDONLY, 0);
    if (fd < 0) return -1;
    char line[1024];
    while (read_line(fd, line, 1024) > 0) {
        char *p = line, *gname = p;
        while (*p && *p != ':') p++;
        if (*p != ':') continue;
        *p++ = '\0';
        while (*p && *p != ':') p++;
        if (*p != ':') continue;
        p++;
        if (str_cmp(gname, name) == 0) {
            *gid = str_to_uint(p);
            sys_close(fd);
            return 0;
        }
    }
    sys_close(fd);
    return -1;
}

/* Format file permissions string like ls -l (e.g. "-rwxr-xr-x") */
static inline void format_mode(unsigned int mode, char *buf) {
    /* File type */
    if (S_ISDIR(mode))       buf[0] = 'd';
    else if (S_ISLNK(mode))  buf[0] = 'l';
    else if (S_ISCHR(mode))  buf[0] = 'c';
    else if (S_ISBLK(mode))  buf[0] = 'b';
    else if (S_ISFIFO(mode)) buf[0] = 'p';
    else if (S_ISSOCK(mode)) buf[0] = 's';
    else                     buf[0] = '-';

    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? ((mode & S_ISUID) ? 's' : 'x') : ((mode & S_ISUID) ? 'S' : '-');
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? ((mode & S_ISGID) ? 's' : 'x') : ((mode & S_ISGID) ? 'S' : '-');
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? ((mode & S_ISVTX) ? 't' : 'x') : ((mode & S_ISVTX) ? 'T' : '-');
    buf[10] = '\0';
}

/* Common _start entry point macro for all tools */
#define QEMT_ENTRY(main_func) \
    __asm__( \
        ".global _start\n" \
        "_start:\n" \
        "    xor %rbp, %rbp\n" \
        "    mov (%rsp), %rdi\n" \
        "    lea 8(%rsp), %rsi\n" \
        "    lea 8(%rsi,%rdi,8), %rdx\n" \
        "    and $-16, %rsp\n" \
        "    call " #main_func "\n" \
        "    mov %eax, %edi\n" \
        "    mov $60, %eax\n" \
        "    syscall\n" \
    );

#endif /* QEMT_IO_H */

/*
 * qoot - Raw Linux syscall wrappers (no glibc dependency)
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
 * These functions use inline assembly to make direct syscalls
 * to the Linux kernel, bypassing any C library.
 */

#ifndef QOOT_SYSCALLS_H
#define QOOT_SYSCALLS_H

/* Syscall numbers for x86_64 */
#define SYS_read        0
#define SYS_write       1
#define SYS_open        2
#define SYS_close       3
#define SYS_stat        4
#define SYS_fstat       5
#define SYS_lstat       6
#define SYS_access      21
#define SYS_getuid      102
#define SYS_getgid      104
#define SYS_setuid      105
#define SYS_setgid      106
#define SYS_geteuid     107
#define SYS_getegid     108
#define SYS_setreuid    113
#define SYS_setregid    114
#define SYS_getgroups   115
#define SYS_setgroups   116
#define SYS_setresuid   117
#define SYS_setresgid   119
#define SYS_execve      59
#define SYS_exit        60
#define SYS_wait4       61
#define SYS_fork        57
#define SYS_ioctl       16
#define SYS_openat      257

/* open() flags */
#define O_RDONLY        0
#define O_WRONLY        1
#define O_RDWR          2
#define O_CREAT         64
#define O_TRUNC         512

/* access() modes */
#define F_OK            0
#define R_OK            4
#define W_OK            2
#define X_OK            1

/* ioctl for terminal */
#define TCGETS          0x5401
#define TCSETS          0x5402
#define TCSETSW         0x5403
#define ECHO            0x0008

/* Null pointer */
#define NULL ((void *)0)

/* Type definitions */
typedef unsigned long size_t;
typedef long ssize_t;
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef unsigned short mode_t;

/* stat structure (simplified for x86_64 Linux) */
struct stat {
    unsigned long st_dev;
    unsigned long st_ino;
    unsigned long st_nlink;
    unsigned int  st_mode;
    unsigned int  st_uid;
    unsigned int  st_gid;
    unsigned int  __pad0;
    unsigned long st_rdev;
    long          st_size;
    long          st_blksize;
    long          st_blocks;
    unsigned long st_atime;
    unsigned long st_atime_nsec;
    unsigned long st_mtime;
    unsigned long st_mtime_nsec;
    unsigned long st_ctime;
    unsigned long st_ctime_nsec;
    long          __unused[3];
};

/* termios structure for terminal manipulation */
struct termios {
    unsigned int c_iflag;
    unsigned int c_oflag;
    unsigned int c_cflag;
    unsigned int c_lflag;
    unsigned char c_line;
    unsigned char c_cc[19];
};

/* Raw syscall wrappers using inline assembly */
static inline long syscall0(long n) {
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(n)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall1(long n, long a1) {
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall2(long n, long a1, long a2) {
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    long ret;
    register long r10 __asm__("r10") = a3;
    (void)r10;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall4(long n, long a1, long a2, long a3, long a4) {
    long ret;
    register long r10 __asm__("r10") = a4;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
        : "rcx", "r11", "memory");
    return ret;
}

/* Convenience wrappers */
static inline ssize_t sys_write(int fd, const void *buf, size_t count) {
    return (ssize_t)syscall3(SYS_write, fd, (long)buf, (long)count);
}

static inline ssize_t sys_read(int fd, void *buf, size_t count) {
    return (ssize_t)syscall3(SYS_read, fd, (long)buf, (long)count);
}

static inline int sys_open(const char *path, int flags, int mode) {
    return (int)syscall3(SYS_open, (long)path, flags, mode);
}

static inline int sys_close(int fd) {
    return (int)syscall1(SYS_close, fd);
}

static inline int sys_stat(const char *path, struct stat *buf) {
    return (int)syscall2(SYS_stat, (long)path, (long)buf);
}

static inline int sys_access(const char *path, int mode) {
    return (int)syscall2(SYS_access, (long)path, mode);
}

static inline uid_t sys_getuid(void) {
    return (uid_t)syscall0(SYS_getuid);
}

static inline gid_t sys_getgid(void) {
    return (gid_t)syscall0(SYS_getgid);
}

static inline uid_t sys_geteuid(void) {
    return (uid_t)syscall0(SYS_geteuid);
}

static inline int sys_setresuid(uid_t ruid, uid_t euid, uid_t suid) {
    return (int)syscall3(SYS_setresuid, ruid, euid, suid);
}

static inline int sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid) {
    return (int)syscall3(SYS_setresgid, rgid, egid, sgid);
}

static inline int sys_setgroups(int size, const gid_t *list) {
    return (int)syscall2(SYS_setgroups, size, (long)list);
}

static inline int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    return (int)syscall3(SYS_execve, (long)path, (long)argv, (long)envp);
}

static inline void sys_exit(int status) {
    syscall1(SYS_exit, status);
    __builtin_unreachable();
}

static inline pid_t sys_fork(void) {
    return (pid_t)syscall0(SYS_fork);
}

static inline pid_t sys_wait4(pid_t pid, int *status, int options, void *rusage) {
    return (pid_t)syscall4(SYS_wait4, pid, (long)status, options, (long)rusage);
}

static inline int sys_ioctl(int fd, unsigned long request, void *arg) {
    return (int)syscall3(SYS_ioctl, fd, (long)request, (long)arg);
}

#endif /* QOOT_SYSCALLS_H */

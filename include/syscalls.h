/*
 * qemt - Raw Linux syscall wrappers (no glibc dependency)
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
 * Complete set of raw Linux x86_64 syscall wrappers for the qemt toolset.
 * Every function uses inline assembly to invoke syscalls directly.
 */

#ifndef QEMT_SYSCALLS_H
#define QEMT_SYSCALLS_H

#include "types.h"

/* ===== x86_64 Syscall Numbers ===== */
#define SYS_read            0
#define SYS_write           1
#define SYS_open            2
#define SYS_close           3
#define SYS_stat            4
#define SYS_fstat           5
#define SYS_lstat           6
#define SYS_poll            7
#define SYS_lseek           8
#define SYS_ioctl          16
#define SYS_access         21
#define SYS_pipe           22
#define SYS_dup2           33
#define SYS_getpid         39
#define SYS_socket         41
#define SYS_connect        42
#define SYS_sendto         44
#define SYS_recvfrom       45
#define SYS_shutdown       48
#define SYS_fork           57
#define SYS_execve         59
#define SYS_exit           60
#define SYS_wait4          61
#define SYS_kill           62
#define SYS_uname          63
#define SYS_fcntl          72
#define SYS_getcwd         79
#define SYS_chdir          80
#define SYS_rename         82
#define SYS_mkdir          83
#define SYS_rmdir          84
#define SYS_link           86
#define SYS_unlink         87
#define SYS_symlink        88
#define SYS_readlink       89
#define SYS_chmod          90
#define SYS_fchmod         91
#define SYS_chown          92
#define SYS_lchown         94
#define SYS_umask          95
#define SYS_gettimeofday   96
#define SYS_sysinfo        99
#define SYS_getuid        102
#define SYS_syslog        103
#define SYS_getgid        104
#define SYS_setuid        105
#define SYS_setgid        106
#define SYS_geteuid       107
#define SYS_getegid       108
#define SYS_getppid       110
#define SYS_setreuid      113
#define SYS_setregid      114
#define SYS_getgroups     115
#define SYS_setgroups     116
#define SYS_setresuid     117
#define SYS_setresgid     119
#define SYS_sethostname   170
#define SYS_reboot        169
#define SYS_mount         165
#define SYS_umount2       166
#define SYS_statfs        137
#define SYS_fstatfs       138
#define SYS_getdents64    217
#define SYS_openat        257
#define SYS_utimensat     280

/* Memory mapping */
#define SYS_mmap            9
#define SYS_munmap         11
#define SYS_mprotect       10

/* Additional syscalls */
#define SYS_ftruncate      77
#define SYS_nanosleep      35
#define SYS_getrandom     318
#define SYS_bind           49
#define SYS_listen         50
#define SYS_accept         43
#define SYS_setsockopt     54
#define SYS_getsockname    51
#define SYS_ttyname         0  /* not a real syscall, use /proc */
#define SYS_sysinfo        99
#define SYS_fcntl          72

/* ===== Raw Syscall Invocations ===== */

static inline long syscall0(long n) {
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret) : "a"(n)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall1(long n, long a1) {
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret) : "a"(n), "D"(a1)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall2(long n, long a1, long a2) {
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret) : "a"(n), "D"(a1), "S"(a2)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile ("syscall"
        : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall4(long n, long a1, long a2, long a3, long a4) {
    long ret;
    register long r10 __asm__("r10") = a4;
    __asm__ volatile ("syscall"
        : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
        : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    __asm__ volatile ("syscall"
        : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory");
    return ret;
}

/* ===== File I/O ===== */

static inline ssize_t sys_read(int fd, void *buf, size_t count) {
    return (ssize_t)syscall3(SYS_read, fd, (long)buf, (long)count);
}

static inline ssize_t sys_write(int fd, const void *buf, size_t count) {
    return (ssize_t)syscall3(SYS_write, fd, (long)buf, (long)count);
}

static inline int sys_open(const char *path, int flags, int mode) {
    return (int)syscall3(SYS_open, (long)path, flags, mode);
}

static inline int sys_close(int fd) {
    return (int)syscall1(SYS_close, fd);
}

static inline off_t sys_lseek(int fd, off_t offset, int whence) {
    return (off_t)syscall3(SYS_lseek, fd, offset, whence);
}

static inline int sys_pipe(int pipefd[2]) {
    return (int)syscall1(SYS_pipe, (long)pipefd);
}

static inline int sys_dup2(int oldfd, int newfd) {
    return (int)syscall2(SYS_dup2, oldfd, newfd);
}

/* ===== File Info ===== */

static inline int sys_stat(const char *path, struct stat *buf) {
    return (int)syscall2(SYS_stat, (long)path, (long)buf);
}

static inline int sys_fstat(int fd, struct stat *buf) {
    return (int)syscall2(SYS_fstat, fd, (long)buf);
}

static inline int sys_lstat(const char *path, struct stat *buf) {
    return (int)syscall2(SYS_lstat, (long)path, (long)buf);
}

static inline int sys_access(const char *path, int mode) {
    return (int)syscall2(SYS_access, (long)path, mode);
}

static inline int sys_statfs(const char *path, struct statfs *buf) {
    return (int)syscall2(SYS_statfs, (long)path, (long)buf);
}

static inline ssize_t sys_readlink(const char *path, char *buf, size_t bufsiz) {
    return (ssize_t)syscall3(SYS_readlink, (long)path, (long)buf, (long)bufsiz);
}

/* ===== Directory Operations ===== */

static inline int sys_getdents64(int fd, void *dirp, unsigned int count) {
    return (int)syscall3(SYS_getdents64, fd, (long)dirp, count);
}

static inline int sys_getcwd(char *buf, size_t size) {
    return (int)syscall2(SYS_getcwd, (long)buf, (long)size);
}

static inline int sys_chdir(const char *path) {
    return (int)syscall1(SYS_chdir, (long)path);
}

static inline int sys_mkdir(const char *path, mode_t mode) {
    return (int)syscall2(SYS_mkdir, (long)path, mode);
}

static inline int sys_rmdir(const char *path) {
    return (int)syscall1(SYS_rmdir, (long)path);
}

/* ===== File Manipulation ===== */

static inline int sys_unlink(const char *path) {
    return (int)syscall1(SYS_unlink, (long)path);
}

static inline int sys_link(const char *oldpath, const char *newpath) {
    return (int)syscall2(SYS_link, (long)oldpath, (long)newpath);
}

static inline int sys_symlink(const char *target, const char *linkpath) {
    return (int)syscall2(SYS_symlink, (long)target, (long)linkpath);
}

static inline int sys_rename(const char *oldpath, const char *newpath) {
    return (int)syscall2(SYS_rename, (long)oldpath, (long)newpath);
}

static inline int sys_chmod(const char *path, mode_t mode) {
    return (int)syscall2(SYS_chmod, (long)path, mode);
}

static inline int sys_chown(const char *path, uid_t owner, gid_t group) {
    return (int)syscall3(SYS_chown, (long)path, owner, group);
}

static inline int sys_lchown(const char *path, uid_t owner, gid_t group) {
    return (int)syscall3(SYS_lchown, (long)path, owner, group);
}

static inline int sys_utimensat(int dirfd, const char *path, const struct timespec *times, int flags) {
    return (int)syscall4(SYS_utimensat, dirfd, (long)path, (long)times, flags);
}

/* ===== User/Group Identity ===== */

static inline uid_t sys_getuid(void)  { return (uid_t)syscall0(SYS_getuid); }
static inline gid_t sys_getgid(void)  { return (gid_t)syscall0(SYS_getgid); }
static inline uid_t sys_geteuid(void) { return (uid_t)syscall0(SYS_geteuid); }
static inline gid_t sys_getegid(void) { return (gid_t)syscall0(SYS_getegid); }
static inline pid_t sys_getpid(void)  { return (pid_t)syscall0(SYS_getpid); }
static inline pid_t sys_getppid(void) { return (pid_t)syscall0(SYS_getppid); }

static inline int sys_setresuid(uid_t r, uid_t e, uid_t s) {
    return (int)syscall3(SYS_setresuid, r, e, s);
}
static inline int sys_setresgid(gid_t r, gid_t e, gid_t s) {
    return (int)syscall3(SYS_setresgid, r, e, s);
}
static inline int sys_setgroups(int size, const gid_t *list) {
    return (int)syscall2(SYS_setgroups, size, (long)list);
}
static inline int sys_getgroups(int size, gid_t *list) {
    return (int)syscall2(SYS_getgroups, size, (long)list);
}

/* ===== Process Management ===== */

static inline pid_t sys_fork(void) {
    return (pid_t)syscall0(SYS_fork);
}

static inline int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    return (int)syscall3(SYS_execve, (long)path, (long)argv, (long)envp);
}

static inline void sys_exit(int status) {
    syscall1(SYS_exit, status);
    __builtin_unreachable();
}

static inline pid_t sys_wait4(pid_t pid, int *status, int options, void *rusage) {
    return (pid_t)syscall4(SYS_wait4, pid, (long)status, options, (long)rusage);
}

static inline int sys_kill(pid_t pid, int sig) {
    return (int)syscall2(SYS_kill, pid, sig);
}

/* ===== System Info ===== */

static inline int sys_uname(struct utsname *buf) {
    return (int)syscall1(SYS_uname, (long)buf);
}

static inline int sys_gettimeofday(struct timeval *tv, struct timezone *tz) {
    return (int)syscall2(SYS_gettimeofday, (long)tv, (long)tz);
}

static inline int sys_syslog(int type, char *buf, int len) {
    return (int)syscall3(SYS_syslog, type, (long)buf, len);
}

static inline int sys_sethostname(const char *name, size_t len) {
    return (int)syscall2(SYS_sethostname, (long)name, (long)len);
}

/* ===== Mount/Reboot ===== */

static inline int sys_mount(const char *src, const char *target,
                            const char *fstype, unsigned long flags, const void *data) {
    return (int)syscall5(SYS_mount, (long)src, (long)target, (long)fstype, (long)flags, (long)data);
}

static inline int sys_umount2(const char *target, int flags) {
    return (int)syscall2(SYS_umount2, (long)target, flags);
}

static inline int sys_reboot(int magic1, int magic2, int cmd, void *arg) {
    return (int)syscall4(SYS_reboot, magic1, magic2, cmd, (long)arg);
}

/* ===== Terminal I/O ===== */

static inline int sys_ioctl(int fd, unsigned long request, void *arg) {
    return (int)syscall3(SYS_ioctl, fd, (long)request, (long)arg);
}

/* ===== Networking ===== */

static inline int sys_socket(int domain, int type, int protocol) {
    return (int)syscall3(SYS_socket, domain, type, protocol);
}

static inline int sys_connect(int sockfd, const void *addr, socklen_t addrlen) {
    return (int)syscall3(SYS_connect, sockfd, (long)addr, addrlen);
}

static inline ssize_t sys_sendto(int sockfd, const void *buf, size_t len, int flags,
                                  const void *dest_addr, socklen_t addrlen) {
    (void)addrlen;
    return (ssize_t)syscall5(SYS_sendto, sockfd, (long)buf, (long)len, flags, (long)dest_addr);
}

static inline ssize_t sys_recvfrom(int sockfd, void *buf, size_t len, int flags,
                                    void *src_addr, socklen_t *addrlen) {
    (void)addrlen;
    return (ssize_t)syscall5(SYS_recvfrom, sockfd, (long)buf, (long)len, flags, (long)src_addr);
}

static inline int sys_shutdown(int sockfd, int how) {
    return (int)syscall2(SYS_shutdown, sockfd, how);
}

static inline int sys_bind(int sockfd, const void *addr, socklen_t addrlen) {
    return (int)syscall3(SYS_bind, sockfd, (long)addr, addrlen);
}

static inline int sys_listen(int sockfd, int backlog) {
    return (int)syscall2(SYS_listen, sockfd, backlog);
}

static inline int sys_accept(int sockfd, void *addr, socklen_t *addrlen) {
    return (int)syscall3(SYS_accept, sockfd, (long)addr, (long)addrlen);
}

static inline int sys_setsockopt(int sockfd, int level, int optname,
                                  const void *optval, socklen_t optlen) {
    return (int)syscall5(SYS_setsockopt, sockfd, level, optname, (long)optval, optlen);
}

static inline int sys_getsockname(int sockfd, void *addr, socklen_t *addrlen) {
    return (int)syscall3(SYS_getsockname, sockfd, (long)addr, (long)addrlen);
}

/* ===== Memory Mapping ===== */

#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED  ((void *)-1)

static inline void *sys_mmap(void *addr, size_t length, int prot, int flags,
                              int fd, off_t offset) {
    /* mmap uses 6 args: syscall6 needed */
    long ret;
    register long r10 __asm__("r10") = (long)flags;
    register long r8  __asm__("r8")  = (long)fd;
    register long r9  __asm__("r9")  = (long)offset;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"((long)SYS_mmap), "D"((long)addr), "S"((long)length),
          "d"((long)prot), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory");
    return (void *)ret;
}

static inline int sys_munmap(void *addr, size_t length) {
    return (int)syscall2(SYS_munmap, (long)addr, (long)length);
}

static inline int sys_ftruncate(int fd, off_t length) {
    return (int)syscall2(SYS_ftruncate, fd, length);
}

static inline int sys_nanosleep(const struct timespec *req, struct timespec *rem) {
    return (int)syscall2(SYS_nanosleep, (long)req, (long)rem);
}

static inline ssize_t sys_getrandom(void *buf, size_t buflen, unsigned int flags) {
    return (ssize_t)syscall3(SYS_getrandom, (long)buf, (long)buflen, (long)flags);
}

static inline int sys_fcntl(int fd, int cmd, long arg) {
    return (int)syscall3(SYS_fcntl, fd, cmd, arg);
}

#endif /* QEMT_SYSCALLS_H */

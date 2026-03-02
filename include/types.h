/*
 * qemt - Type definitions (no glibc dependency)
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
 * Fundamental type definitions for x86_64 Linux, independent of any libc.
 */

#ifndef QEMT_TYPES_H
#define QEMT_TYPES_H

/* Null pointer */
#define NULL ((void *)0)

/* Boolean */
#define true  1
#define false 0

/* Fundamental types */
typedef unsigned long  size_t;
typedef long           ssize_t;
typedef long           off_t;
typedef int            pid_t;
typedef unsigned int   uid_t;
typedef unsigned int   gid_t;
typedef unsigned short mode_t;
typedef unsigned long  dev_t;
typedef unsigned long  ino_t;
typedef unsigned long  nlink_t;
typedef long           blksize_t;
typedef long           blkcnt_t;
typedef long           time_t;
typedef long           suseconds_t;
typedef unsigned long  socklen_t;

/* Limits */
#define PATH_MAX   4096
#define NAME_MAX   255
#define LINE_MAX   4096
#define UINT_MAX   0xFFFFFFFFU

/* open() flags */
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0100
#define O_EXCL      0200
#define O_TRUNC     01000
#define O_APPEND    02000
#define O_NONBLOCK  04000
#define O_DIRECTORY 0200000
#define O_NOFOLLOW  0400000
#define O_CLOEXEC   02000000

/* access() modes */
#define F_OK  0
#define R_OK  4
#define W_OK  2
#define X_OK  1

/* lseek whence */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* File type bits from stat st_mode */
#define S_IFMT   0170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

/* Permission bits */
#define S_ISUID  04000
#define S_ISGID  02000
#define S_ISVTX  01000
#define S_IRUSR  00400
#define S_IWUSR  00200
#define S_IXUSR  00100
#define S_IRGRP  00040
#define S_IWGRP  00020
#define S_IXGRP  00010
#define S_IROTH  00004
#define S_IWOTH  00002
#define S_IXOTH  00001

/* Signal numbers */
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1  10
#define SIGSEGV  11
#define SIGUSR2  12
#define SIGPIPE  13
#define SIGALRM  14
#define SIGTERM  15
#define SIGCHLD  17
#define SIGCONT  18
#define SIGSTOP  19
#define SIGTSTP  20

/* wait flags */
#define WNOHANG    1
#define WUNTRACED  2

#define WIFEXITED(s)    (!((s) & 0x7f))
#define WEXITSTATUS(s)  (((s) & 0xff00) >> 8)
#define WIFSIGNALED(s)  (((s) & 0x7f) > 0 && ((s) & 0x7f) < 0x7f)
#define WTERMSIG(s)     ((s) & 0x7f)

/* mount flags */
#define MS_RDONLY       1
#define MS_NOSUID       2
#define MS_NODEV        4
#define MS_NOEXEC       8
#define MS_REMOUNT      32
#define MS_BIND         4096

/* reboot commands */
#define LINUX_REBOOT_MAGIC1         0xfee1dead
#define LINUX_REBOOT_MAGIC2         672274793
#define LINUX_REBOOT_CMD_RESTART    0x01234567
#define LINUX_REBOOT_CMD_HALT       0xCDEF0123
#define LINUX_REBOOT_CMD_POWER_OFF  0x4321FEDC

/* ioctl terminal */
#define TCGETS   0x5401
#define TCSETS   0x5402
#define TCSETSW  0x5403
#define ECHO     0x0008
#define TIOCGWINSZ 0x5413

/* Socket */
#define AF_INET     2
#define AF_INET6   10
#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define IPPROTO_TCP 6
#define SOL_SOCKET  1
#define SO_REUSEADDR 2

/* stat structure (x86_64 Linux) */
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
    unsigned long st_atime_sec;
    unsigned long st_atime_nsec;
    unsigned long st_mtime_sec;
    unsigned long st_mtime_nsec;
    unsigned long st_ctime_sec;
    unsigned long st_ctime_nsec;
    long          __unused[3];
};

/* linux_dirent64 for getdents64 */
struct linux_dirent64 {
    unsigned long  d_ino;
    long           d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

/* dirent types */
#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2
#define DT_DIR      4
#define DT_BLK      6
#define DT_REG      8
#define DT_LNK     10
#define DT_SOCK    12

/* utsname structure */
struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

/* termios structure */
struct termios {
    unsigned int c_iflag;
    unsigned int c_oflag;
    unsigned int c_cflag;
    unsigned int c_lflag;
    unsigned char c_line;
    unsigned char c_cc[19];
};

/* winsize for TIOCGWINSZ */
struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

/* timeval */
struct timeval {
    long tv_sec;
    long tv_usec;
};

/* timezone */
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

/* timespec for utimensat */
struct timespec {
    long tv_sec;
    long tv_nsec;
};

#define UTIME_NOW  ((1L << 30) - 1L)
#define UTIME_OMIT ((1L << 30) - 2L)

/* AT constants */
#define AT_FDCWD            -100
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_EMPTY_PATH       0x1000

/* statfs structure for df */
struct statfs {
    long f_type;
    long f_bsize;
    long f_blocks;
    long f_bfree;
    long f_bavail;
    long f_files;
    long f_ffree;
    long __f_fsid[2];
    long f_namelen;
    long f_frsize;
    long f_flags;
    long f_spare[4];
};

/* sockaddr_in for networking */
struct sockaddr_in {
    unsigned short sin_family;
    unsigned short sin_port;
    unsigned int   sin_addr;
    unsigned char  sin_zero[8];
};

#endif /* QEMT_TYPES_H */

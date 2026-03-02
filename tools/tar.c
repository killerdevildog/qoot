/*
 * tar - tape archive create/extract (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

/* tar header (512 bytes, POSIX ustar) */
struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

static unsigned long tar_oct_to_ulong(const char *s, int len) {
    unsigned long val = 0;
    for (int i = 0; i < len && s[i] >= '0' && s[i] <= '7'; i++)
        val = (val << 3) | (unsigned long)(s[i] - '0');
    return val;
}

static void tar_ulong_to_oct(char *buf, int len, unsigned long val) {
    buf[len - 1] = '\0';
    for (int i = len - 2; i >= 0; i--) {
        buf[i] = '0' + (char)(val & 7);
        val >>= 3;
    }
}

static unsigned int tar_checksum(const struct tar_header *h) {
    const unsigned char *p = (const unsigned char *)h;
    unsigned int sum = 256; /* treat chksum field as spaces */
    for (int i = 0; i < 148; i++) sum += p[i];
    for (int i = 156; i < 512; i++) sum += p[i];
    return sum;
}

static int tar_is_zero_block(const char *block) {
    for (int i = 0; i < 512; i++)
        if (block[i] != 0) return 0;
    return 1;
}

static void tar_build_path(char *out, const struct tar_header *h) {
    out[0] = '\0';
    if (h->prefix[0]) {
        str_ncpy(out, h->prefix, 155);
        out[155] = '\0';
        str_cat(out, "/");
    }
    int off = (int)str_len(out);
    str_ncpy(out + off, h->name, 100);
    out[off + 100] = '\0';
}

/* Create parent directories for a path */
static void tar_mkdirs(const char *path) {
    char buf[PATH_MAX];
    str_cpy(buf, path);
    for (char *p = buf + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            sys_mkdir(buf, 0755);
            *p = '/';
        }
    }
}

static int tar_extract(int in_fd, int verbose) {
    char block[512];
    int zero_count = 0;

    while (1) {
        ssize_t r = sys_read(in_fd, block, 512);
        if (r <= 0) break;
        if (r < 512) { eprint("tar: short read\n"); return 1; }

        if (tar_is_zero_block(block)) {
            zero_count++;
            if (zero_count >= 2) break;
            continue;
        }
        zero_count = 0;

        struct tar_header *h = (struct tar_header *)block;

        /* Verify checksum */
        unsigned int stored = (unsigned int)tar_oct_to_ulong(h->chksum, 8);
        unsigned int computed = tar_checksum(h);
        if (stored != computed) {
            eprint("tar: invalid header checksum\n");
            return 1;
        }

        char filepath[PATH_MAX];
        tar_build_path(filepath, h);
        unsigned long size = tar_oct_to_ulong(h->size, 12);
        unsigned int mode = (unsigned int)tar_oct_to_ulong(h->mode, 8);

        if (verbose) {
            print(filepath);
            print("\n");
        }

        switch (h->typeflag) {
        case '5': /* directory */
            tar_mkdirs(filepath);
            sys_mkdir(filepath, mode ? (mode_t)mode : 0755);
            break;

        case '2': /* symlink */
        {
            char target[101];
            str_ncpy(target, h->linkname, 100);
            target[100] = '\0';
            tar_mkdirs(filepath);
            sys_unlink(filepath);
            sys_symlink(target, filepath);
            break;
        }

        case '1': /* hard link */
        {
            char target[101];
            str_ncpy(target, h->linkname, 100);
            target[100] = '\0';
            tar_mkdirs(filepath);
            sys_unlink(filepath);
            sys_link(target, filepath);
            break;
        }

        default: /* regular file ('0' or '\0') */
        {
            tar_mkdirs(filepath);
            int fd = sys_open(filepath, O_WRONLY | O_CREAT | O_TRUNC,
                              mode ? (int)mode : 0644);
            if (fd < 0) {
                eprint("tar: cannot create '");
                eprint(filepath);
                eprint("'\n");
                /* skip data blocks */
                unsigned long blocks = (size + 511) / 512;
                for (unsigned long b = 0; b < blocks; b++)
                    sys_read(in_fd, block, 512);
                break;
            }

            unsigned long remaining = size;
            while (remaining > 0) {
                r = sys_read(in_fd, block, 512);
                if (r < 512) { sys_close(fd); return 1; }
                unsigned long chunk = remaining > 512 ? 512 : remaining;
                write_all(fd, block, chunk);
                remaining -= chunk;
            }
            sys_close(fd);
            sys_chmod(filepath, mode ? (mode_t)mode : 0644);
            break;
        }
        }
    }
    return 0;
}

static int tar_list(int in_fd) {
    char block[512];
    int zero_count = 0;

    while (1) {
        ssize_t r = sys_read(in_fd, block, 512);
        if (r <= 0) break;
        if (r < 512) break;

        if (tar_is_zero_block(block)) {
            if (++zero_count >= 2) break;
            continue;
        }
        zero_count = 0;

        struct tar_header *h = (struct tar_header *)block;
        char filepath[PATH_MAX];
        tar_build_path(filepath, h);
        unsigned long size = tar_oct_to_ulong(h->size, 12);

        print(filepath);
        print("\n");

        /* Skip data blocks */
        unsigned long blocks = (size + 511) / 512;
        for (unsigned long b = 0; b < blocks; b++)
            sys_read(in_fd, block, 512);
    }
    return 0;
}

static int tar_write_header(int fd, const char *path, struct stat *st) {
    struct tar_header h;
    mem_set(&h, 0, 512);

    /* Handle long names with prefix */
    size_t plen = str_len(path);
    if (plen > 100) {
        /* Try to split at a '/' */
        for (int i = (int)plen - 100; i < (int)plen && i < 155; i++) {
            if (path[i] == '/') {
                str_ncpy(h.prefix, path, (size_t)i);
                str_ncpy(h.name, path + i + 1, 100);
                goto name_done;
            }
        }
        str_ncpy(h.name, path, 100);
    } else {
        str_ncpy(h.name, path, 100);
    }
name_done:

    tar_ulong_to_oct(h.mode, 8, st->st_mode & 07777);
    tar_ulong_to_oct(h.uid, 8, st->st_uid);
    tar_ulong_to_oct(h.gid, 8, st->st_gid);
    tar_ulong_to_oct(h.mtime, 12, st->st_mtime_sec);

    if (S_ISDIR(st->st_mode)) {
        h.typeflag = '5';
        tar_ulong_to_oct(h.size, 12, 0);
    } else if (S_ISLNK(st->st_mode)) {
        h.typeflag = '2';
        char link[100];
        ssize_t n = sys_readlink(path, link, 99);
        if (n > 0) { link[n] = '\0'; str_ncpy(h.linkname, link, 100); }
        tar_ulong_to_oct(h.size, 12, 0);
    } else {
        h.typeflag = '0';
        tar_ulong_to_oct(h.size, 12, (unsigned long)st->st_size);
    }

    mem_cpy(h.magic, "ustar", 6);
    h.version[0] = '0'; h.version[1] = '0';

    /* Compute checksum */
    mem_set(h.chksum, ' ', 8);
    unsigned int chk = tar_checksum(&h);
    tar_ulong_to_oct(h.chksum, 7, chk);
    h.chksum[7] = ' ';

    return write_all(fd, &h, 512);
}

static int tar_create_recursive(int out_fd, const char *path, int verbose);

static int tar_add_dir(int out_fd, const char *dirpath, int verbose) {
    int dfd = sys_open(dirpath, O_RDONLY | O_DIRECTORY, 0);
    if (dfd < 0) return -1;

    char buf[4096];
    int nread;
    while ((nread = sys_getdents64(dfd, buf, sizeof(buf))) > 0) {
        int pos = 0;
        while (pos < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + pos);
            if (str_cmp(d->d_name, ".") != 0 && str_cmp(d->d_name, "..") != 0) {
                char child[PATH_MAX];
                str_cpy(child, dirpath);
                if (child[str_len(child) - 1] != '/') str_cat(child, "/");
                str_cat(child, d->d_name);
                tar_create_recursive(out_fd, child, verbose);
            }
            pos += d->d_reclen;
        }
    }
    sys_close(dfd);
    return 0;
}

static int tar_create_recursive(int out_fd, const char *path, int verbose) {
    struct stat st;
    if (sys_lstat(path, &st) < 0) {
        eprint("tar: cannot stat '"); eprint(path); eprint("'\n");
        return -1;
    }

    if (verbose) { print(path); print("\n"); }
    tar_write_header(out_fd, path, &st);

    if (S_ISREG(st.st_mode) && st.st_size > 0) {
        int fd = sys_open(path, O_RDONLY, 0);
        if (fd < 0) return -1;

        char buf[512];
        long remaining = st.st_size;
        while (remaining > 0) {
            ssize_t r = sys_read(fd, buf, remaining > 512 ? 512 : (size_t)remaining);
            if (r <= 0) break;
            /* Pad to 512 bytes */
            if (r < 512) mem_set(buf + r, 0, (size_t)(512 - r));
            write_all(out_fd, buf, 512);
            remaining -= r;
        }
        sys_close(fd);
    }

    if (S_ISDIR(st.st_mode))
        tar_add_dir(out_fd, path, verbose);

    return 0;
}

int tar_main(int argc, char **argv, char **envp) {
    (void)envp;
    int create = 0, extract = 0, list = 0, verbose = 0;
    const char *file = NULL;
    int file_args_start = -1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' || (i == 1 && file_args_start < 0)) {
            const char *opts = argv[i][0] == '-' ? argv[i] + 1 : argv[i];
            for (int j = 0; opts[j]; j++) {
                switch (opts[j]) {
                case 'c': create = 1; break;
                case 'x': extract = 1; break;
                case 't': list = 1; break;
                case 'v': verbose = 1; break;
                case 'f':
                    if (!opts[j + 1]) {
                        if (i + 1 < argc) file = argv[++i];
                    }
                    goto opts_done;
                default: break;
                }
            }
            opts_done:;
        } else {
            if (file_args_start < 0) file_args_start = i;
        }
    }

    if (!create && !extract && !list) {
        print("Usage: tar [-cxtv] [-f file] [files...]\n");
        print("  -c  Create archive\n");
        print("  -x  Extract archive\n");
        print("  -t  List archive contents\n");
        print("  -v  Verbose\n");
        print("  -f  Archive file (default: stdin/stdout)\n");
        print("\nTo extract .tar.gz: gunzip file.tar.gz && tar xf file.tar\n");
        print("Or: zcat file.tar.gz | tar x\n");
        return 1;
    }

    if (extract || list) {
        int fd = 0; /* stdin */
        if (file) {
            fd = sys_open(file, O_RDONLY, 0);
            if (fd < 0) { eprint("tar: cannot open '"); eprint(file); eprint("'\n"); return 1; }
        }
        int ret = list ? tar_list(fd) : tar_extract(fd, verbose);
        if (file) sys_close(fd);
        return ret;
    }

    if (create) {
        int fd = 1; /* stdout */
        if (file) {
            fd = sys_open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { eprint("tar: cannot create '"); eprint(file); eprint("'\n"); return 1; }
        }

        for (int i = (file_args_start >= 0 ? file_args_start : argc); i < argc; i++)
            tar_create_recursive(fd, argv[i], verbose);

        /* Write two zero blocks as end of archive */
        char zero[512];
        mem_set(zero, 0, 512);
        write_all(fd, zero, 512);
        write_all(fd, zero, 512);

        if (file) sys_close(fd);
        return 0;
    }

    return 1;
}

QEMT_ENTRY(tar_main)

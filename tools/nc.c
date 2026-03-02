/*
 * nc - netcat: TCP connect/listen (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static unsigned int parse_ip(const char *s) {
    unsigned int ip = 0;
    for (int i = 0; i < 4; i++) {
        unsigned int octet = 0;
        while (*s >= '0' && *s <= '9') { octet = octet * 10 + (unsigned int)(*s - '0'); s++; }
        ip = (ip << 8) | (octet & 0xff);
        if (*s == '.') s++;
    }
    /* Convert to network byte order (big endian) */
    return ((ip & 0xff) << 24) | (((ip >> 8) & 0xff) << 16) |
           (((ip >> 16) & 0xff) << 8) | ((ip >> 24) & 0xff);
}

static unsigned short htons(unsigned short v) {
    return (unsigned short)((v >> 8) | (v << 8));
}

/* Simple bidirectional relay between stdin/stdout and socket */
static int relay(int sock_fd) {
    /* Use poll-like approach with non-blocking reads */
    unsigned char buf[4096];

    /* Make both stdin and socket non-blocking */
    sys_fcntl(0, 4 /* F_SETFL */, O_NONBLOCK);
    sys_fcntl(sock_fd, 4, O_NONBLOCK);

    for (;;) {
        /* Read from stdin, write to socket */
        ssize_t r = sys_read(0, buf, sizeof(buf));
        if (r > 0) {
            const unsigned char *p = buf;
            long remaining = r;
            while (remaining > 0) {
                ssize_t w = sys_write(sock_fd, p, (size_t)remaining);
                if (w <= 0) return 0;
                p += w;
                remaining -= w;
            }
        } else if (r == 0) {
            /* EOF on stdin */
            sys_shutdown(sock_fd, 1);
            break;
        }

        /* Read from socket, write to stdout */
        r = sys_read(sock_fd, buf, sizeof(buf));
        if (r > 0) {
            write_all(1, buf, (size_t)r);
        } else if (r == 0) {
            /* Remote closed */
            break;
        }

        /* Small sleep to avoid busy loop */
        struct timespec ts = {0, 10000000}; /* 10ms */
        sys_nanosleep(&ts, NULL);
    }
    return 0;
}

int nc_main(int argc, char **argv, char **envp) {
    (void)envp;
    int listen_mode = 0;
    const char *host = NULL;
    int port = 0;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-l") == 0) {
            listen_mode = 1;
        } else if (str_cmp(argv[i], "-h") == 0 || str_cmp(argv[i], "--help") == 0) {
            print("Usage: nc [-l] HOST PORT\n");
            print("       nc -l PORT\n");
            print("  -l  Listen mode\n");
            print("\nExamples:\n");
            print("  nc 192.168.1.1 80     Connect to host:port\n");
            print("  nc -l 8080            Listen on port\n");
            return 0;
        } else if (argv[i][0] != '-') {
            if (!host && !listen_mode) {
                host = argv[i];
            } else {
                port = (int)str_to_uint(argv[i]);
            }
        }
    }

    if (listen_mode && port == 0 && host) {
        /* nc -l PORT (host is actually port) */
        port = (int)str_to_uint(host);
        host = NULL;
    }

    if (port <= 0 || port > 65535) {
        eprint("nc: invalid port\n");
        return 1;
    }

    if (listen_mode) {
        /* Server mode */
        int sock = sys_socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { eprint("nc: socket failed\n"); return 1; }

        int one = 1;
        sys_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        struct sockaddr_in addr;
        mem_set(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((unsigned short)port);
        addr.sin_addr = 0; /* INADDR_ANY */

        if (sys_bind(sock, &addr, sizeof(addr)) < 0) {
            eprint("nc: bind failed\n");
            sys_close(sock);
            return 1;
        }

        if (sys_listen(sock, 1) < 0) {
            eprint("nc: listen failed\n");
            sys_close(sock);
            return 1;
        }

        eprint("nc: listening on port ");
        char pbuf[8]; uint_to_str(pbuf, (unsigned int)port); eprint(pbuf);
        eprint("\n");

        socklen_t addrlen = sizeof(addr);
        int client = sys_accept(sock, &addr, &addrlen);
        if (client < 0) {
            eprint("nc: accept failed\n");
            sys_close(sock);
            return 1;
        }

        eprint("nc: connection accepted\n");
        relay(client);
        sys_close(client);
        sys_close(sock);
    } else {
        /* Client mode */
        if (!host) { eprint("nc: missing host\n"); return 1; }

        int sock = sys_socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { eprint("nc: socket failed\n"); return 1; }

        struct sockaddr_in addr;
        mem_set(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((unsigned short)port);
        addr.sin_addr = parse_ip(host);

        if (sys_connect(sock, &addr, sizeof(addr)) < 0) {
            eprint("nc: connection refused\n");
            sys_close(sock);
            return 1;
        }

        relay(sock);
        sys_close(sock);
    }

    return 0;
}

QEMT_ENTRY(nc_main)

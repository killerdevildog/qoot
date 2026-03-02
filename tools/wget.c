/*
 * wget - HTTP file downloader via raw sockets (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Supports HTTP/1.0 GET requests. No HTTPS (that would require a TLS stack).
 * For emergency recovery scenarios where you need to fetch files.
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

/* htons for network byte order */
static unsigned short htons(unsigned short x) {
    return (unsigned short)((x >> 8) | (x << 8));
}

/* Simple DNS resolution via /etc/hosts and /etc/resolv.conf */
/* For simplicity, use numeric IPs or /etc/hosts lookup */
static int resolve_host(const char *host, unsigned int *ip_out) {
    /* Check if it's already an IP address */
    unsigned int a = 0, b = 0, c = 0, d = 0;
    const char *p = host;
    int dots = 0;
    unsigned int parts[4] = {0,0,0,0};
    int pi = 0;

    for (int i = 0; host[i]; i++) {
        if (host[i] == '.') dots++;
        else if (!is_digit(host[i])) { dots = -1; break; }
    }

    if (dots == 3) {
        /* Parse IP directly */
        while (*p && pi < 4) {
            parts[pi] = 0;
            while (*p && *p != '.') { parts[pi] = parts[pi] * 10 + (*p - '0'); p++; }
            if (*p == '.') p++;
            pi++;
        }
        a = parts[0]; b = parts[1]; c = parts[2]; d = parts[3];
        *ip_out = (a) | (b << 8) | (c << 16) | (d << 24);
        return 0;
    }

    /* Try /etc/hosts */
    int fd = sys_open("/etc/hosts", O_RDONLY, 0);
    if (fd >= 0) {
        char buf[4096];
        ssize_t n = sys_read(fd, buf, sizeof(buf) - 1);
        sys_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            char *line = buf;
            while (*line) {
                char *nl = str_chr(line, '\n');
                if (nl) *nl = '\0';
                /* Skip comments */
                if (*line != '#' && *line != '\0') {
                    char *sp = line;
                    while (*sp && *sp != ' ' && *sp != '\t') sp++;
                    if (*sp) {
                        *sp++ = '\0';
                        while (*sp == ' ' || *sp == '\t') sp++;
                        /* Check if hostname matches */
                        char *hname = sp;
                        while (*hname) {
                            char *end = hname;
                            while (*end && *end != ' ' && *end != '\t') end++;
                            char saved = *end;
                            *end = '\0';
                            if (str_cmp(hname, host) == 0) {
                                /* Parse IP from line start */
                                return resolve_host(line, ip_out);
                            }
                            *end = saved;
                            hname = end;
                            while (*hname == ' ' || *hname == '\t') hname++;
                        }
                    }
                }
                if (nl) line = nl + 1; else break;
            }
        }
    }

    /* Try DNS resolution via UDP socket to resolver */
    /* Read /etc/resolv.conf for nameserver */
    unsigned int dns_ip = 0;
    fd = sys_open("/etc/resolv.conf", O_RDONLY, 0);
    if (fd >= 0) {
        char rbuf[2048];
        ssize_t n = sys_read(fd, rbuf, sizeof(rbuf) - 1);
        sys_close(fd);
        if (n > 0) {
            rbuf[n] = '\0';
            char *line = rbuf;
            while (*line) {
                char *nl = str_chr(line, '\n');
                if (nl) *nl = '\0';
                if (starts_with(line, "nameserver ")) {
                    resolve_host(line + 11, &dns_ip);
                    break;
                }
                if (nl) line = nl + 1; else break;
            }
        }
    }
    if (dns_ip == 0) dns_ip = 0x08080808; /* 8.8.8.8 fallback */

    /* Build DNS query */
    unsigned char query[512];
    int qlen = 0;

    /* Header */
    query[qlen++] = 0x00; query[qlen++] = 0x01; /* ID */
    query[qlen++] = 0x01; query[qlen++] = 0x00; /* Flags: standard query, recursion desired */
    query[qlen++] = 0x00; query[qlen++] = 0x01; /* QDCOUNT = 1 */
    query[qlen++] = 0x00; query[qlen++] = 0x00; /* ANCOUNT */
    query[qlen++] = 0x00; query[qlen++] = 0x00; /* NSCOUNT */
    query[qlen++] = 0x00; query[qlen++] = 0x00; /* ARCOUNT */

    /* Encode hostname as DNS labels */
    p = host;
    while (*p) {
        const char *dot = str_chr(p, '.');
        int labellen = dot ? (int)(dot - p) : (int)str_len(p);
        query[qlen++] = (unsigned char)labellen;
        for (int i = 0; i < labellen; i++) query[qlen++] = (unsigned char)p[i];
        if (dot) p = dot + 1; else break;
    }
    query[qlen++] = 0x00; /* End of name */
    query[qlen++] = 0x00; query[qlen++] = 0x01; /* QTYPE = A */
    query[qlen++] = 0x00; query[qlen++] = 0x01; /* QCLASS = IN */

    /* Send DNS query */
    int sock = sys_socket(2, 2, 17); /* AF_INET, SOCK_DGRAM, IPPROTO_UDP */
    if (sock < 0) {
        eprint("wget: cannot create DNS socket\n");
        return -1;
    }

    struct sockaddr_in dns_addr;
    mem_set(&dns_addr, 0, sizeof(dns_addr));
    dns_addr.sin_family = 2;
    dns_addr.sin_port = htons(53);
    dns_addr.sin_addr = dns_ip;

    sys_sendto(sock, query, qlen, 0, &dns_addr, sizeof(dns_addr));

    unsigned char resp[512];
    int rlen = sys_recvfrom(sock, resp, sizeof(resp), 0, NULL, NULL);
    sys_shutdown(sock, 2);

    if (rlen < 12) {
        eprint("wget: DNS resolution failed for '"); eprint(host); eprint("'\n");
        return -1;
    }

    /* Parse DNS response - skip to answer section */
    int offset = 12;
    /* Skip question section */
    while (offset < rlen && resp[offset] != 0) {
        if ((resp[offset] & 0xC0) == 0xC0) { offset += 2; break; }
        offset += resp[offset] + 1;
    }
    if (offset < rlen && resp[offset] == 0) offset++;
    offset += 4; /* QTYPE + QCLASS */

    /* Parse answer records */
    int ancount = (resp[6] << 8) | resp[7];
    for (int i = 0; i < ancount && offset < rlen; i++) {
        /* Name (possibly compressed) */
        if ((resp[offset] & 0xC0) == 0xC0) offset += 2;
        else { while (offset < rlen && resp[offset]) offset += resp[offset] + 1; offset++; }

        if (offset + 10 > rlen) break;
        int rtype = (resp[offset] << 8) | resp[offset + 1];
        int rdlen = (resp[offset + 8] << 8) | resp[offset + 9];
        offset += 10;

        if (rtype == 1 && rdlen == 4) { /* A record */
            *ip_out = resp[offset] | (resp[offset+1] << 8) | (resp[offset+2] << 16) | (resp[offset+3] << 24);
            return 0;
        }
        offset += rdlen;
    }

    eprint("wget: cannot resolve '"); eprint(host); eprint("'\n");
    return -1;
}

static int parse_url(const char *url, char *host, size_t hsz, int *port, char *path, size_t psz) {
    const char *p = url;
    if (starts_with(p, "http://")) p += 7;
    else if (starts_with(p, "https://")) {
        eprint("wget: HTTPS not supported (no TLS without glibc)\n");
        return -1;
    }

    /* Extract host */
    const char *slash = str_chr(p, '/');
    const char *colon = str_chr(p, ':');
    size_t hlen;

    *port = 80;
    if (colon && (!slash || colon < slash)) {
        hlen = (size_t)(colon - p);
        *port = (int)str_to_uint(colon + 1);
    } else if (slash) {
        hlen = (size_t)(slash - p);
    } else {
        hlen = str_len(p);
    }

    if (hlen >= hsz) hlen = hsz - 1;
    mem_cpy(host, p, hlen);
    host[hlen] = '\0';

    if (slash) str_ncpy(path, slash, psz - 1);
    else str_cpy(path, "/");

    return 0;
}

int wget_main(int argc, char **argv, char **envp) {
    (void)envp;
    const char *output = NULL;
    int quiet = 0;
    int first = 1;

    if (argc > 1 && (str_cmp(argv[1], "-h") == 0 || str_cmp(argv[1], "--help") == 0)) {
        print("Usage: wget [-q] [-O file] url\n");
        print("  -O file  Save to file (- for stdout)\n");
        print("  -q       Quiet mode\n");
        print("Note: HTTP only (no HTTPS without a TLS library)\n");
        return 0;
    }

    while (first < argc && argv[first][0] == '-') {
        if (str_cmp(argv[first], "-O") == 0 && first + 1 < argc) {
            output = argv[first + 1]; first += 2;
        } else if (str_cmp(argv[first], "-q") == 0) {
            quiet = 1; first++;
        } else { first++; }
    }

    if (first >= argc) {
        eprint("wget: missing URL\n");
        return 1;
    }

    const char *url = argv[first];
    char host[256], path[2048];
    int port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) < 0)
        return 1;

    /* Determine output filename */
    char out_name[256];
    if (!output) {
        const char *slash = str_rchr(path, '/');
        const char *fname = slash ? slash + 1 : path;
        if (fname[0] == '\0' || str_cmp(fname, "/") == 0) {
            str_cpy(out_name, "index.html");
        } else {
            str_ncpy(out_name, fname, sizeof(out_name) - 1);
        }
        output = out_name;
    }

    if (!quiet) {
        eprint("Resolving "); eprint(host); eprint("...\n");
    }

    unsigned int ip;
    if (resolve_host(host, &ip) < 0) return 1;

    if (!quiet) {
        eprint("Connecting to "); eprint(host); eprint(":");
        char pbuf[8]; uint_to_str(pbuf, (unsigned int)port); eprint(pbuf);
        eprint("...\n");
    }

    int sock = sys_socket(2, 1, 6); /* AF_INET, SOCK_STREAM, IPPROTO_TCP */
    if (sock < 0) { eprint("wget: socket failed\n"); return 1; }

    struct sockaddr_in addr;
    mem_set(&addr, 0, sizeof(addr));
    addr.sin_family = 2;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr = ip;

    if (sys_connect(sock, &addr, sizeof(addr)) < 0) {
        eprint("wget: connection failed\n");
        return 1;
    }

    /* Build HTTP request */
    char request[4096];
    str_cpy(request, "GET ");
    str_cat(request, path);
    str_cat(request, " HTTP/1.0\r\nHost: ");
    str_cat(request, host);
    str_cat(request, "\r\nUser-Agent: qemt-wget/1.0\r\nConnection: close\r\n\r\n");

    ssize_t reqlen = (ssize_t)str_len(request);
    ssize_t sent = 0;
    while (sent < reqlen) {
        ssize_t n = sys_write(sock, request + sent, (size_t)(reqlen - sent));
        if (n <= 0) { eprint("wget: send failed\n"); return 1; }
        sent += n;
    }

    if (!quiet) eprint("HTTP request sent, awaiting response...\n");

    /* Read response */
    char buf[8192];
    ssize_t total_read = 0;
    int header_done = 0;
    int out_fd = -1;
    int status_code = 0;
    long content_length = -1;
    long body_received = 0;

    while (1) {
        ssize_t n = sys_recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
        if (n <= 0) break;

        if (!header_done) {
            /* Look for end of headers */
            total_read += n;
            for (ssize_t i = 0; i < n - 3; i++) {
                if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n') {
                    /* Parse status line */
                    buf[i] = '\0';
                    char *status_line = buf;
                    char *sp = str_chr(status_line, ' ');
                    if (sp) status_code = (int)str_to_uint(sp + 1);

                    /* Look for Content-Length */
                    char *hdr = buf;
                    while (*hdr) {
                        if (starts_with(hdr, "Content-Length: ") || starts_with(hdr, "content-length: ")) {
                            content_length = str_to_long(hdr + 16);
                        }
                        while (*hdr && *hdr != '\n') hdr++;
                        if (*hdr == '\n') hdr++;
                    }

                    if (!quiet) {
                        eprint("HTTP "); char sbuf[8]; uint_to_str(sbuf, (unsigned int)status_code); eprint(sbuf);
                        if (content_length >= 0) {
                            eprint(" ["); long_to_str(sbuf, content_length); eprint(sbuf); eprint(" bytes]");
                        }
                        eprint("\n");
                    }

                    if (status_code >= 400) {
                        eprint("wget: server returned error "); char sbuf2[8]; uint_to_str(sbuf2, (unsigned int)status_code); eprint(sbuf2); eprint("\n");
                        sys_shutdown(sock, 2);
                        return 1;
                    }

                    /* Open output */
                    if (str_cmp(output, "-") == 0) {
                        out_fd = 1;
                    } else {
                        out_fd = sys_open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (out_fd < 0) {
                            eprint("wget: cannot create '"); eprint(output); eprint("'\n");
                            sys_shutdown(sock, 2);
                            return 1;
                        }
                    }

                    /* Write remaining body data */
                    ssize_t body_start = i + 4;
                    ssize_t body_len = n - body_start;
                    if (body_len > 0) {
                        sys_write(out_fd, buf + body_start, (size_t)body_len);
                        body_received += body_len;
                    }
                    header_done = 1;
                    break;
                }
            }
        } else {
            /* Body data */
            sys_write(out_fd, buf, (size_t)n);
            body_received += n;
        }
    }

    sys_shutdown(sock, 2);

    if (out_fd > 1) sys_close(out_fd);

    if (!quiet) {
        eprint("Saved '"); eprint(output); eprint("' [");
        char sbuf[21]; long_to_str(sbuf, body_received); eprint(sbuf);
        eprint(" bytes]\n");
    }

    return 0;
}

QEMT_ENTRY(wget_main)

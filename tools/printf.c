/*
 * printf - formatted output (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

static void put_char(char c) {
    sys_write(1, &c, 1);
}

static void put_string(const char *s) {
    sys_write(1, s, str_len(s));
}

static void put_padded_str(const char *s, int width, int left_align) {
    int len = (int)str_len(s);
    int pad = width > len ? width - len : 0;
    if (!left_align) for (int i = 0; i < pad; i++) put_char(' ');
    put_string(s);
    if (left_align) for (int i = 0; i < pad; i++) put_char(' ');
}

static void put_padded_num(const char *num, int width, int left_align, int zero_pad) {
    int len = (int)str_len(num);
    int pad = width > len ? width - len : 0;
    char pc = zero_pad ? '0' : ' ';
    /* Handle negative with zero padding */
    if (zero_pad && num[0] == '-') {
        put_char('-');
        num++;
        len--;
        pad = width > len + 1 ? width - len - 1 : 0;
    }
    if (!left_align) for (int i = 0; i < pad; i++) put_char(pc);
    put_string(num);
    if (left_align) for (int i = 0; i < pad; i++) put_char(' ');
}

int printf_main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc < 2) {
        print("Usage: printf FORMAT [ARG...]\n");
        return 1;
    }

    const char *fmt = argv[1];
    int argi = 2;

    for (const char *p = fmt; *p; p++) {
        if (*p == '\\') {
            p++;
            switch (*p) {
            case 'n': put_char('\n'); break;
            case 't': put_char('\t'); break;
            case 'r': put_char('\r'); break;
            case '\\': put_char('\\'); break;
            case '0': {
                /* Octal */
                unsigned char val = 0;
                for (int i = 0; i < 3 && p[1] >= '0' && p[1] <= '7'; i++) {
                    p++;
                    val = (val << 3) | (unsigned char)(*p - '0');
                }
                put_char((char)val);
                break;
            }
            default:
                put_char('\\');
                if (*p) put_char(*p);
                break;
            }
        } else if (*p == '%') {
            p++;
            int left_align = 0, zero_pad = 0, width = 0;

            if (*p == '-') { left_align = 1; p++; }
            if (*p == '0') { zero_pad = 1; p++; }
            while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }

            const char *arg = (argi < argc) ? argv[argi++] : "";

            switch (*p) {
            case 's':
                put_padded_str(arg, width, left_align);
                break;
            case 'd': case 'i': {
                long val = str_to_long(arg);
                char buf[21];
                long_to_str(buf, val);
                put_padded_num(buf, width, left_align, zero_pad);
                break;
            }
            case 'u': {
                unsigned long val = 0;
                const char *s = arg;
                while (*s >= '0' && *s <= '9') { val = val * 10 + (unsigned long)(*s - '0'); s++; }
                char buf[21];
                ulong_to_str(buf, val);
                put_padded_num(buf, width, left_align, zero_pad);
                break;
            }
            case 'x': case 'X': {
                unsigned long val;
                const char *s = arg;
                if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
                    /* Parse as hex literal */
                    val = 0;
                    s += 2;
                    while (*s) {
                        if (*s >= '0' && *s <= '9') val = (val << 4) | (unsigned long)(*s - '0');
                        else if (*s >= 'a' && *s <= 'f') val = (val << 4) | (unsigned long)(*s - 'a' + 10);
                        else if (*s >= 'A' && *s <= 'F') val = (val << 4) | (unsigned long)(*s - 'A' + 10);
                        else break;
                        s++;
                    }
                } else {
                    /* Parse as decimal, output as hex */
                    val = (unsigned long)str_to_long(arg);
                }
                char buf[17];
                ulong_to_hex(buf, val, *p == 'X');
                put_padded_num(buf, width, left_align, zero_pad);
                break;
            }
            case 'o': {
                unsigned long val = (unsigned long)str_to_long(arg);
                char buf[23];
                /* octal conversion */
                if (val == 0) { buf[0] = '0'; buf[1] = '\0'; }
                else {
                    char tmp[23]; int i = 0;
                    while (val > 0) { tmp[i++] = '0' + (char)(val & 7); val >>= 3; }
                    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
                    buf[i] = '\0';
                }
                put_padded_num(buf, width, left_align, zero_pad);
                break;
            }
            case 'c':
                put_char(arg[0] ? arg[0] : '\0');
                break;
            case '%':
                put_char('%');
                argi--; /* didn't consume an arg */
                break;
            default:
                put_char('%');
                if (*p) put_char(*p);
                argi--;
                break;
            }
        } else {
            put_char(*p);
        }
    }

    return 0;
}

QEMT_ENTRY(printf_main)

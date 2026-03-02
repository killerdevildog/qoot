/*
 * reboot - reboot or power off system (no glibc dependency)
 * Part of qemt - Q Emergency Tools
 *
 * Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "../include/io.h"

int reboot_main(int argc, char **argv, char **envp) {
    (void)envp;
    int do_poweroff = 0;
    int force = 0;

    if (argc > 1 && str_cmp(argv[1], "-h") == 0) {
        print("Usage: reboot [-f] [-p]\n");
        print("  -f  Force (don't call shutdown scripts)\n");
        print("  -p  Power off instead of reboot\n");
        print("  Also: poweroff (symlink) for power off\n");
        return 0;
    }

    /* Check if called as "poweroff" */
    const char *name = argv[0];
    const char *slash = str_rchr(name, '/');
    if (slash) name = slash + 1;
    if (str_cmp(name, "poweroff") == 0) do_poweroff = 1;

    for (int i = 1; i < argc; i++) {
        if (str_cmp(argv[i], "-f") == 0) force = 1;
        else if (str_cmp(argv[i], "-p") == 0) do_poweroff = 1;
    }

    if (sys_getuid() != 0) {
        eprint("reboot: must be run as root\n");
        return 1;
    }

    if (!force) {
        /* Sync filesystems */
        __asm__ volatile("syscall" : : "a"(162) : "rcx", "r11", "memory"); /* SYS_sync */
    }

    if (do_poweroff) {
        print("System powering off...\n");
        sys_reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF, NULL);
    } else {
        print("System rebooting...\n");
        sys_reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL);
    }

    eprint("reboot: failed\n");
    return 1;
}

QEMT_ENTRY(reboot_main)

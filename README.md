# qemt — Q Emergency Tools

**A complete Unix toolkit built entirely on raw Linux syscalls — zero glibc dependency.**

qemt is a full set of essential Unix command-line tools for Linux x86_64 that operate without *any* C library. Every binary uses inline assembly to make direct system calls to the Linux kernel. When your system's glibc is broken, these tools still work.

## The Problem

I've been experimenting with updating and modifying the glibc framework on my system, and I've bricked it — multiple times. When glibc breaks, almost everything on the system stops working. Not just `sudo` — `ls`, `cat`, `cp`, `mv`, `grep`, `mount`, the shell itself — anything dynamically linked against glibc is dead. You're completely locked out of your own machine with no way to fix it short of booting a live USB.

**qemt exists so that never happens again.** It's a complete emergency toolkit — 48 essential Unix tools plus a sudo replacement — all statically compiled with zero library dependencies. As long as the kernel is running, qemt works. Toggle it on with `qemt on`, fix your system, toggle it off with `qemt off`.

## Tools Included (48+)

| Category | Tools |
|----------|-------|
| **File Operations** | `ls` `cat` `cp` `mv` `rm` `mkdir` `rmdir` `touch` `find` `du` |
| **Text Processing** | `grep` `head` `tail` `sort` `uniq` `wc` `tr` `tee` |
| **File Info** | `stat` `chmod` `chown` `ln` `readlink` `basename` `dirname` |
| **System Info** | `whoami` `id` `which` `pwd` `uname` `hostname` `date` `env` `echo` `uptime` `free` |
| **Process Mgmt** | `ps` `kill` `sleep` |
| **System Admin** | `mount` `umount` `df` `dmesg` `reboot` `poweroff` |
| **Privilege Escalation** | `sudo` (glibc-free sudo replacement) |
| **Network** | `wget` (HTTP, raw sockets) |
| **Shell** | `sh` (emergency shell with pipes, redirects, PATH lookup) |
| **Utility** | `clear` |

Every single one uses **raw Linux syscalls** via inline assembly. No glibc. No musl. No libc at all.

## Building

```bash
make
```

Just needs `gcc`. No libraries, no libc headers.

### Verify no dependencies

```bash
make verify
# Every tool should show: "not a dynamic executable"
```

## Installing

```bash
sudo make install-rescue
```

This installs:
1. All tools to `/usr/local/qemt/` with their original names (`ls`, `cat`, `sudo`, etc.)
2. The `qemt.sh` shell function to `/usr/local/share/qemt/qemt.sh`
3. Default config to `/etc/qemt.conf` (for the sudo tool)
4. The `sudo` binary gets setuid root (`4755`)

### Setup

Add to your `~/.bashrc` or `~/.profile`:

```bash
source /usr/local/share/qemt/qemt.sh
```

## Usage — Switching On and Off

```bash
# Activate qemt (shadows all system tools with glibc-free versions)
qemt on

# Now all commands use the emergency toolkit:
ls /root          # uses /usr/local/qemt/ls
sudo apt update   # uses /usr/local/qemt/sudo
cat /etc/fstab    # uses /usr/local/qemt/cat
grep root /etc/passwd  # uses /usr/local/qemt/grep

# Deactivate (restore normal system tools)
qemt off

# Check status
qemt status

# List available tools
qemt list
```

### How `qemt on/off` Works

It's simple and non-invasive — just PATH manipulation:

- **`qemt on`** — Prepends `/usr/local/qemt` to your `PATH` and saves the original. All commands now resolve to the qemt versions first.
- **`qemt off`** — Restores your original `PATH`. System tools are back.

No symlinks created, no system files modified, no services restarted. Just an environment variable change in your current shell.

## sudo Configuration

The qemt `sudo` replacement reads `/etc/qemt.conf`:

```bash
# Allow a specific user (password prompt)
myuser

# Allow user without password
myuser NOPASSWD

# Allow ALL users
ALL

# Allow ALL users without password
ALL NOPASSWD
```

## How It Works

Every tool uses **no C library at all**. All operations go through raw Linux syscalls via inline assembly:

| Operation | Syscall Used |
|-----------|-------------|
| Read/write files | `SYS_read`, `SYS_write`, `SYS_open`, `SYS_close` |
| Directory listing | `SYS_getdents64` |
| File metadata | `SYS_stat`, `SYS_lstat`, `SYS_fstat`, `SYS_access` |
| File manipulation | `SYS_rename`, `SYS_unlink`, `SYS_mkdir`, `SYS_rmdir`, `SYS_chmod`, `SYS_chown` |
| Process control | `SYS_fork`, `SYS_execve`, `SYS_wait4`, `SYS_kill` |
| Privilege escalation | `SYS_setresuid`, `SYS_setresgid`, `SYS_setgroups` |
| System info | `SYS_uname`, `SYS_sysinfo`, `SYS_clock_gettime` |
| Networking | `SYS_socket`, `SYS_connect`, `SYS_sendto`, `SYS_recvfrom` |
| Terminal I/O | `SYS_ioctl` |
| Mount/unmount | `SYS_mount`, `SYS_umount2` |
| Process start | Custom `_start` entry point (no `crt0`) |

The entry point for every binary is a hand-written `_start` in assembly that extracts `argc`, `argv`, and `envp` from the stack — exactly as the kernel leaves them.

## Architecture

```
include/
├── syscalls.h   # Raw syscall wrappers (inline asm, 60+ syscalls)
├── types.h      # Type definitions (no libc headers)
├── string.h     # String utilities
├── io.h         # I/O helpers (file reading, line parsing)
└── auth.h       # sudo authentication & authorization

tools/
├── sudo.c       # Privilege escalation (setuid root)
├── ls.c         # Directory listing
├── cat.c        # File concatenation
├── cp.c         # File copy
├── mv.c         # File move/rename
├── rm.c         # File removal
├── grep.c       # Pattern matching
├── sh.c         # Emergency shell
├── wget.c       # HTTP downloads (raw sockets)
├── ...          # 48 tools total
└── (each is a standalone binary)

qemt.sh          # Shell function for on/off switching
qemt.conf.example # sudo config template
```

## Security Notes

- `sudo` binary **must** be setuid root (`chmod 4755`)
- Authorization checked against `/etc/qemt.conf`
- Clean, minimal environment constructed for child processes
- Supplementary groups cleared before privilege transition
- Password memory zeroed after use
- Uses `setresuid`/`setresgid` for complete credential transition
- `qemt on/off` only modifies PATH — no system files touched

### Limitations

- **x86_64 Linux only** — syscall numbers and ABI are architecture-specific
- Password verification against `/etc/shadow` hashes requires SHA-512 crypt (planned). Use `NOPASSWD` mode or the password prompt acts as a gate.
- No PAM integration (by design — PAM requires glibc)
- `wget` supports HTTP only (no HTTPS without a TLS library)
- `sh` is minimal — supports pipes, redirects, PATH, and builtins but not full bash features

## Use Cases

1. **Broken glibc recovery** — The primary reason this exists. When you brick glibc experimenting with updates, `qemt on` gives you a full working toolkit to fix it without a live USB.
2. **glibc development/testing** — If you're actively working on or testing glibc builds, install qemt first as your safety net.
3. **Minimal containers** — Scratch/distroless containers with no libc need tools too.
4. **Embedded systems** — Tiny footprint, complete toolkit.
5. **Education** — Learn how Unix tools work at the lowest level — raw syscalls, no abstraction.

## License

MIT — Copyright (c) 2026 Quaylyn Rimer

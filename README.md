# qoot

**A sudo alternative built entirely on raw Linux syscalls — zero glibc dependency.**

qoot is a minimal privilege escalation tool for Linux x86_64 that operates without *any* C library. It uses inline assembly to make direct system calls to the Linux kernel, making it ideal as an emergency/backup sudo when your system's glibc is broken, missing, or compromised.

## Why?

If glibc gets corrupted, updated badly, or you're working in a minimal environment without it, standard `sudo` won't work — it dynamically links against glibc. **qoot** is your safety net:

- **Zero library dependencies** — statically compiled, no glibc/musl/anything
- **Raw syscalls only** — talks directly to the Linux kernel via `syscall` instruction
- **Tiny binary** — typically under 20KB
- **Simple config** — single file at `/etc/qoot.conf`
- **Setuid-based** — standard Unix privilege escalation model

## Building

```bash
make
```

That's it. Just needs `gcc` — no libraries, no headers from libc.

### Verify no dependencies

```bash
make verify
# Should show: "not a dynamic executable" or "statically linked"
```

## Installing

```bash
sudo make install
```

This will:
1. Copy the binary to `/usr/local/bin/qoot`
2. Set ownership to `root:root` with setuid bit (`4755`)
3. Install a default config to `/etc/qoot.conf` (if none exists)

## Configuration

Edit `/etc/qoot.conf`:

```bash
# Allow a specific user (password prompt)
myuser

# Allow a user without password
myuser NOPASSWD

# Allow ALL users (password prompt)
ALL

# Allow ALL users without password
ALL NOPASSWD
```

## Usage

```bash
# Run a command as root
qoot apt update

# Spawn a root shell
qoot -s

# Run as a specific user
qoot -u www-data nginx -t

# Version info
qoot -v
```

## How It Works

qoot uses **no C library at all**. Every operation is performed through raw Linux syscalls via inline assembly:

| Operation | Syscall Used |
|-----------|-------------|
| Read files | `SYS_read`, `SYS_open` |
| Write output | `SYS_write` |
| Check permissions | `SYS_access`, `SYS_getuid`, `SYS_geteuid` |
| Set credentials | `SYS_setresuid`, `SYS_setresgid`, `SYS_setgroups` |
| Execute commands | `SYS_execve` |
| Terminal I/O | `SYS_ioctl` (for password input with echo disabled) |
| Process start | Custom `_start` entry point (no `crt0`) |

The binary entry point is a hand-written `_start` in assembly that extracts `argc`, `argv`, and `envp` from the stack — exactly as the kernel leaves them.

## Architecture

```
src/
├── qoot.c       # Main program + _start entry point
├── syscalls.h   # Raw syscall wrappers (inline asm)
├── string.h     # String utilities (no libc)
└── auth.h       # Authentication & authorization
```

## Security Notes

- The binary **must** be installed setuid root (`chmod 4755`)
- Authorization is checked against `/etc/qoot.conf`
- A clean, minimal environment is constructed for child processes
- Supplementary groups are cleared before privilege transition
- Password memory is zeroed after use
- Uses `setresuid`/`setresgid` for complete credential transition

### Limitations

- **x86_64 Linux only** — syscall numbers and ABI are architecture-specific
- Password verification against `/etc/shadow` hashes requires implementing SHA-512 crypt natively (planned). For now, use `NOPASSWD` mode or the password prompt acts as a basic gate.
- No PAM integration (by design — PAM requires glibc)

## Use Cases

1. **Broken glibc recovery** — When a bad update bricks glibc, qoot still works
2. **Minimal containers** — Scratch/distroless containers with no libc
3. **Embedded systems** — Tiny footprint privilege escalation
4. **Security hardening** — Reduce attack surface by eliminating libc dependency
5. **Education** — Learn how Linux syscalls work at the lowest level

## License

MIT

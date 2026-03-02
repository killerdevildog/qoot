# qemt - Q Emergency Tools
# A complete set of Unix tools with ZERO glibc dependency
# Every binary uses raw Linux x86_64 syscalls via inline assembly
#
# Copyright (c) 2026 Quaylyn Rimer. MIT License.

CC = gcc
CFLAGS = -nostdlib -static -no-pie -O2 -Wall -Wextra -Werror \
         -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables \
         -fno-unwind-tables -I include
LDFLAGS = -nostdlib -static -no-pie

BUILD_DIR = build
TOOLS_DIR = tools
INCLUDE_DIR = include

# All tools to build (auto-detected from tools/*.c)
TOOL_SRCS = $(wildcard $(TOOLS_DIR)/*.c)
TOOL_NAMES = $(basename $(notdir $(TOOL_SRCS)))
TOOL_BINS = $(addprefix $(BUILD_DIR)/,$(TOOL_NAMES))

# Shared headers
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

# Aliases (symlinks to existing tools)
ALIASES = poweroff

.PHONY: all clean install install-rescue uninstall verify list help

all: $(TOOL_BINS) aliases
	@echo ""
	@echo "========================================="
	@echo " qemt - Q Emergency Tools"
	@echo " Built $(words $(TOOL_NAMES)) tools + $(words $(ALIASES)) aliases"
	@echo "========================================="
	@echo ""
	@echo "All binaries in $(BUILD_DIR)/"
	@echo "Total size: $$(du -sh $(BUILD_DIR) | cut -f1)"
	@echo ""
	@echo "To install: sudo make install-rescue"
	@echo "Then:       source /usr/local/share/qemt/qemt.sh"
	@echo "            qemt on"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Pattern rule: build each tool from its source
$(BUILD_DIR)/%: $(TOOLS_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@printf "  %-12s %s bytes\n" "$*" "$$(stat -c%s $@ 2>/dev/null || stat -f%z $@)"

# Create symlink aliases
aliases: $(BUILD_DIR)/reboot
	@ln -sf reboot $(BUILD_DIR)/poweroff
	@echo "  Created alias: poweroff -> reboot"

# Verify all binaries are static
verify: all
	@echo ""
	@echo "=== Verification: No glibc Dependencies ==="
	@failed=0; \
	for bin in $(TOOL_BINS); do \
		result=$$(ldd $$bin 2>&1); \
		if echo "$$result" | grep -q "not a dynamic"; then \
			printf "  ✓ %s\n" "$$(basename $$bin)"; \
		else \
			printf "  ✗ %s - HAS DEPENDENCIES\n" "$$(basename $$bin)"; \
			failed=1; \
		fi; \
	done; \
	echo ""; \
	if [ "$$failed" = "0" ]; then \
		echo "All $(words $(TOOL_NAMES)) tools verified: ZERO glibc dependency"; \
	else \
		echo "WARNING: Some tools have unexpected dependencies!"; \
	fi

# List all available tools
list:
	@echo "qemt - Q Emergency Tools ($(words $(TOOL_NAMES)) tools)"
	@echo ""
	@echo "Core File Operations:"
	@echo "  ls cat cp mv rm mkdir rmdir touch find du"
	@echo ""
	@echo "Text Processing:"
	@echo "  grep head tail sort uniq wc tr tee"
	@echo ""
	@echo "File Information:"
	@echo "  stat chmod chown ln readlink basename dirname"
	@echo ""
	@echo "System Information:"
	@echo "  whoami id which pwd uname hostname date env echo uptime free"
	@echo ""
	@echo "Process Management:"
	@echo "  ps kill sleep"
	@echo ""
	@echo "System Administration:"
	@echo "  mount umount df dmesg reboot (poweroff)"
	@echo ""
	@echo "Privilege Escalation:"
	@echo "  sudo (glibc-free sudo replacement)"
	@echo ""
	@echo "Network:"
	@echo "  wget (HTTP only, raw sockets)"
	@echo ""
	@echo "Shell:"
	@echo "  sh (minimal emergency shell with pipes, redirects, PATH)"
	@echo ""
	@echo "Utility:"
	@echo "  clear"

# Install all tools with qemt- prefix (side-by-side with system tools)
install: all
	@echo "Installing qemt tools to /usr/local/bin..."
	@for bin in $(TOOL_BINS); do \
		name=$$(basename $$bin); \
		if [ "$$name" = "sudo" ]; then \
			cp $$bin /usr/local/bin/qemt-sudo; \
			chown root:root /usr/local/bin/qemt-sudo; \
			chmod 4755 /usr/local/bin/qemt-sudo; \
			echo "  qemt-sudo (setuid root)"; \
		else \
			cp $$bin /usr/local/bin/qemt-$$name; \
			chmod 755 /usr/local/bin/qemt-$$name; \
			echo "  qemt-$$name"; \
		fi; \
	done
	@for alias in $(ALIASES); do \
		ln -sf qemt-reboot /usr/local/bin/qemt-$$alias; \
		echo "  qemt-$$alias -> qemt-reboot"; \
	done
	@mkdir -p /usr/local/share/qemt
	@cp qemt.sh /usr/local/share/qemt/qemt.sh
	@chmod 644 /usr/local/share/qemt/qemt.sh
	@echo ""
	@echo "Installed! Tools available as qemt-<tool> (e.g., qemt-ls, qemt-cat)"
	@echo "sudo replacement available as: qemt-sudo"

# Install to rescue directory using original names (ls, cat, sudo, etc.)
# This is the primary install method - used with 'qemt on' to shadow system tools
install-rescue: all
	@echo "Installing qemt rescue toolkit to /usr/local/qemt/..."
	@mkdir -p /usr/local/qemt
	@for bin in $(TOOL_BINS); do \
		name=$$(basename $$bin); \
		cp $$bin /usr/local/qemt/$$name; \
		if [ "$$name" = "sudo" ]; then \
			chown root:root /usr/local/qemt/sudo; \
			chmod 4755 /usr/local/qemt/sudo; \
			echo "  sudo (setuid root)"; \
		else \
			chmod 755 /usr/local/qemt/$$name; \
			echo "  $$name"; \
		fi; \
	done
	@for alias in $(ALIASES); do \
		ln -sf reboot /usr/local/qemt/$$alias; \
		echo "  $$alias -> reboot"; \
	done
	@mkdir -p /usr/local/share/qemt
	@cp qemt.sh /usr/local/share/qemt/qemt.sh
	@chmod 644 /usr/local/share/qemt/qemt.sh
	@if [ ! -f /etc/qemt.conf ]; then \
		cp qemt.conf.example /etc/qemt.conf; \
		chmod 644 /etc/qemt.conf; \
		echo "  Installed default config: /etc/qemt.conf"; \
	fi
	@echo ""
	@echo "========================================="
	@echo " qemt rescue toolkit installed!"
	@echo "========================================="
	@echo ""
	@echo "  Tools:  /usr/local/qemt/"
	@echo "  Config: /etc/qemt.conf"
	@echo "  Script: /usr/local/share/qemt/qemt.sh"
	@echo ""
	@echo "  To activate, add to your ~/.bashrc:"
	@echo "    source /usr/local/share/qemt/qemt.sh"
	@echo ""
	@echo "  Then run:"
	@echo "    qemt on     # activate (shadows system tools)"
	@echo "    qemt off    # deactivate (restores system tools)"
	@echo "    qemt status # check state"

uninstall:
	@echo "Removing qemt tools..."
	@rm -f /usr/local/bin/qemt-sudo
	@for bin in $(TOOL_BINS); do \
		rm -f /usr/local/bin/qemt-$$(basename $$bin); \
	done
	@for alias in $(ALIASES); do \
		rm -f /usr/local/bin/qemt-$$alias; \
	done
	@rm -rf /usr/local/qemt
	@rm -rf /usr/local/share/qemt
	@echo "Removed. Note: /etc/qemt.conf was NOT removed."

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "qemt Makefile targets:"
	@echo "  make           - Build all tools"
	@echo "  make verify    - Build and verify no glibc deps"
	@echo "  make list      - List all available tools"
	@echo "  make install   - Install to /usr/local/bin (qemt-<tool>)"
	@echo "  make install-rescue - Install to /usr/local/qemt/"
	@echo "  make uninstall - Remove all installed tools"
	@echo "  make clean     - Remove build directory"

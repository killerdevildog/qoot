# qoot - glibc-free sudo
# Compiles with ZERO library dependencies using raw Linux syscalls

CC = gcc
CFLAGS = -nostdlib -static -no-pie -O2 -Wall -Wextra -Werror \
         -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables \
         -fno-unwind-tables
LDFLAGS = -nostdlib -static -no-pie

SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/qoot

SRC = $(SRC_DIR)/qoot.c
HEADERS = $(SRC_DIR)/syscalls.h $(SRC_DIR)/string.h $(SRC_DIR)/auth.h

.PHONY: all clean install uninstall verify

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRC) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
	@echo ""
	@echo "Build complete: $(TARGET)"
	@echo "Binary size: $$(stat -c%s $(TARGET) 2>/dev/null || stat -f%z $(TARGET)) bytes"
	@echo ""
	@echo "Verifying no glibc dependency:"
	@ldd $(TARGET) 2>&1 || true
	@echo ""
	@echo "To install (requires root):"
	@echo "  sudo make install"

# Verify the binary has no dynamic dependencies
verify: $(TARGET)
	@echo "=== Binary Info ==="
	@file $(TARGET)
	@echo ""
	@echo "=== Dynamic Dependencies ==="
	@ldd $(TARGET) 2>&1; true
	@echo ""
	@echo "=== Size ==="
	@ls -lh $(TARGET)
	@echo ""
	@echo "=== Symbols (should show raw syscall usage) ==="
	@nm $(TARGET) 2>/dev/null | head -20 || objdump -t $(TARGET) | head -20

# Install with setuid root permissions
install: $(TARGET)
	@echo "Installing qoot to /usr/local/bin..."
	install -o root -g root -m 4755 $(TARGET) /usr/local/bin/qoot
	@if [ ! -f /etc/qoot.conf ]; then \
		echo "Installing default config to /etc/qoot.conf..."; \
		install -o root -g root -m 0644 qoot.conf.example /etc/qoot.conf; \
	else \
		echo "/etc/qoot.conf already exists, not overwriting."; \
	fi
	@echo "Done. qoot installed with setuid root."

uninstall:
	rm -f /usr/local/bin/qoot
	@echo "Removed /usr/local/bin/qoot"
	@echo "Note: /etc/qoot.conf was NOT removed. Remove manually if desired."

clean:
	rm -rf $(BUILD_DIR)

TERMUX_BASE_DIR ?= /data/data/com.termux/files
CFLAGS += -Wall -Wextra -Werror -Wshadow -O2
C_SOURCE := src/termux-exec.c src/exec-variants.c
CLANG_FORMAT := clang-format --sort-includes --style="{ColumnLimit: 120}" $(C_SOURCE)
CLANG_TIDY ?= clang-tidy

libtermux-exec.so: $(C_SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) $(C_SOURCE) -DTERMUX_PREFIX=\"$(TERMUX_PREFIX)\" -DTERMUX_BASE_DIR=\"$(TERMUX_BASE_DIR)\" -shared -fPIC -o libtermux-exec.so

clean:
	rm -f libtermux-exec.so tests/*-actual test-binary

install: libtermux-exec.so
	install libtermux-exec.so $(DESTDIR)$(PREFIX)/lib/libtermux-exec.so

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libtermux-exec.so

on-device-tests: libtermux-exec.so
	@LD_PRELOAD=${CURDIR}/libtermux-exec.so ./run-tests.sh

format:
	$(CLANG_FORMAT) -i $(C_SOURCE)

check:
	$(CLANG_FORMAT) --dry-run $(C_SOURCE)
	$(CLANG_TIDY) -warnings-as-errors='*' $(C_SOURCE) -- -DTERMUX_BASE_DIR=\"$(TERMUX_BASE_DIR)\"

test-binary: $(C_SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) $(C_SOURCE) -g -fsanitize=address -fno-omit-frame-pointer -DUNIT_TEST=1 -DTERMUX_BASE_DIR=\"$(TERMUX_BASE_DIR)\" -o test-binary

deb: libtermux-exec.so
	termux-create-package termux-exec-debug.json

unit-test: test-binary
	./test-binary

.PHONY: deb clean install uninstall test format check-format test

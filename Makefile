TERMUX_PREFIX := /data/data/com.termux/files/usr
TERMUX_BASE_DIR := /data/data/com.termux/files
CFLAGS += -Wall -Wextra -Werror -Oz

libtermux-exec.so: termux-exec.c
	$(CC) $(CFLAGS) $(LDFLAGS) termux-exec.c -DTERMUX_PREFIX=\"$(TERMUX_PREFIX)\" -DTERMUX_BASE_DIR=\"$(TERMUX_BASE_DIR)\" -shared -fPIC -o libtermux-exec.so

install: libtermux-exec.so
	install libtermux-exec.so $(DESTDIR)$(PREFIX)/lib/libtermux-exec.so

uninstall:
	rm -f $(PREFIX)/lib/libtermux-exec.so

test: libtermux-exec.so
	@LD_PRELOAD=${CURDIR}/libtermux-exec.so ./run-tests.sh

clean:
	rm -f libtermux-exec.so tests/*-actual

.PHONY: clean install test uninstall

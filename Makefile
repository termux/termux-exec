CFLAGS += -Wall -Wextra -Werror -Oz

libtermux-exec.so: termux-exec.c
	$(CC) $(CFLAGS) $(LDFLAGS) termux-exec.c -shared -fPIC -o libtermux-exec.so

install: libtermux-exec.so
	install libtermux-exec.so $(PREFIX)/lib/libtermux-exec.so

uninstall:
	rm -f $(PREFIX)/lib/libtermux-exec.so

test: libtermux-exec.so
	@LD_PRELOAD=${CURDIR}/libtermux-exec.so ./run-tests.sh

clean:
	rm -f libtermux-exec.so tests/*-actual

.PHONY: clean install test uninstall

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	CFLAGS += -Oz
else
	CFLAGS += -Os
endif

libtermux-exec.so: termux-exec.c termux_rewrite_executable.c
	$(CC) $(CFLAGS) $^ -shared -fPIC -o $@

termux-rewrite-exe: termux-rewrite-exe.c termux_rewrite_executable.c
	$(CC) $(CFLAGS) $^ -o $@

install: libtermux-exec.so
	install libtermux-exec.so $(PREFIX)/lib/libtermux-exec.so

uninstall:
	rm -f $(PREFIX)/lib/libtermux-exec.so

test: libtermux-exec.so
	@LD_PRELOAD=${CURDIR}/libtermux-exec.so ./run-tests.sh

clean:
	rm -f libtermux-exec.so termux-rewrite-exe tests/*-actual

.PHONY: clean install test uninstall

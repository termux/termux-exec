export TERMUX_EXEC__VERSION ?= 1.0
export TERMUX__NAME ?= Termux# Default value: `Termux`
export TERMUX_APP__PACKAGE_NAME ?= com.termux# Default value: `com.termux`
export TERMUX__ROOTFS ?= /data/data/$(TERMUX_APP__PACKAGE_NAME)/files# Default value: `/data/data/com.termux/files`
export TERMUX__PREFIX ?= $(TERMUX__ROOTFS)/usr# Default value: `/data/data/com.termux/files/usr`
export TERMUX_ENV__S_ROOT ?= TERMUX_# Default value: `TERMUX_`
export TERMUX_ENV__S_TERMUX ?= $(TERMUX_ENV__S_ROOT)_# Default value: `TERMUX__`
export TERMUX_ENV__SE_TERMUX ?= $$$(TERMUX_ENV__S_TERMUX)# Default value: `$TERMUX__`
export TERMUX_ENV__S_TERMUX_API_API ?= $(TERMUX_ENV__S_ROOT)API_API__# Default value: `TERMUX_API_API__`
export TERMUX_ENV__SE_TERMUX_API_APP ?= $$$(TERMUX_ENV__S_TERMUX_API_API)# Default value: `$TERMUX_API_API__`
export TERMUX_ENV__S_TERMUX_EXEC ?= $(TERMUX_ENV__S_ROOT)EXEC__# Default value: `TERMUX_EXEC__`
export TERMUX_ENV__SE_TERMUX_EXEC ?= $$$(TERMUX_ENV__S_TERMUX_EXEC)# Default value: `$TERMUX_EXEC__`
export TERMUX_ENV__S_TERMUX_EXEC_TESTS ?= $(TERMUX_ENV__S_ROOT)EXEC_TESTS__# Default value: `TERMUX_EXEC_TESTS__`
export TERMUX_ENV__SE_TERMUX_EXEC_TESTS ?= $$$(TERMUX_ENV__S_TERMUX_EXEC_TESTS)# Default value: `$TERMUX_EXEC_TESTS__`
export TERMUX_APP__NAMESPACE ?= $(TERMUX_APP__PACKAGE_NAME)# Default value: `com.termux`
export TERMUX_APP__SHELL_ACTIVITY__COMPONENT_NAME ?= $(TERMUX_APP__PACKAGE_NAME)/$(TERMUX_APP__NAMESPACE).app.TermuxActivity# Default value: `com.termux/com.termux.app.TermuxActivity`
export TERMUX_APP__SHELL_SERVICE__COMPONENT_NAME ?= $(TERMUX_APP__PACKAGE_NAME)/$(TERMUX_APP__NAMESPACE).app.TermuxService# Default value: `com.termux/com.termux.app.TermuxService`

ifeq ($(TERMUX__ROOTFS), /)
ifeq ($(TERMUX__ROOTFS), $(TERMUX__PREFIX))
	TERMUX_BIN_DIR := /bin# Default value: `/bin`
endif
endif
TERMUX_BIN_DIR ?= $(TERMUX__PREFIX)/bin# Default value: `/data/data/com.termux/files/usr/bin`

TERMUX_EXEC_TESTS_API_LEVEL ?=


CFLAGS += -Wall -Wextra -Werror -Wshadow -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong
FSANTIZE_FLAGS += -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer
CLANG_FORMAT := clang-format --sort-includes --style="{ColumnLimit: 120}" $(C_SOURCE)
CLANG_TIDY ?= clang-tidy
C_SOURCE := \
	src/data/assert_utils.c \
	src/data/data_utils.c \
	src/exec/exec.c \
	src/exec/exec_variants.c \
	src/file/file_utils.c \
	src/logger/logger.c \
	src/os/safe_strerror.c \
	src/os/selinux_utils.c \
	src/termux/termux_env.c \
	src/termux/termux_files.c
TERMUX_CONSTANTS := \
	-DTERMUX_EXEC__VERSION=\"$(TERMUX_EXEC__VERSION)\" \
	-DTERMUX__NAME=\"$(TERMUX__NAME)\" \
    -DTERMUX__ROOTFS=\"$(TERMUX__ROOTFS)\" \
    -DTERMUX__PREFIX=\"$(TERMUX__PREFIX)\" \
    -DTERMUX_BIN_DIR=\"$(TERMUX_BIN_DIR)\" \
    -DTERMUX_ENV__S_TERMUX=\"$(TERMUX_ENV__S_TERMUX)\" \
    -DTERMUX_ENV__S_TERMUX_EXEC=\"$(TERMUX_ENV__S_TERMUX_EXEC)\" \
    -DTERMUX_ENV__S_TERMUX_EXEC_TESTS=\"$(TERMUX_ENV__S_TERMUX_EXEC_TESTS)\"


define replace-termux-constants
	@echo "Creating $1"
	@sed -e 's%[@]TERMUX_APP__PACKAGE_NAME[@]%$(TERMUX_APP__PACKAGE_NAME)%g' \
		 -e 's%[@]TERMUX__PREFIX[@]%$(TERMUX__PREFIX)%g' \
		 -e 's%[@]TERMUX_ENV__S_TERMUX[@]%$(TERMUX_ENV__S_TERMUX)%g' \
		 -e 's%[@]TERMUX_ENV__SE_TERMUX[@]%$(TERMUX_ENV__SE_TERMUX)%g' \
		 -e 's%[@]TERMUX_ENV__S_TERMUX_API_API[@]%$(TERMUX_ENV__S_TERMUX_API_API)%g' \
		 -e 's%[@]TERMUX_ENV__SE_TERMUX_API_APP[@]%$(TERMUX_ENV__SE_TERMUX_API_APP)%g' \
		 -e 's%[@]TERMUX_ENV__S_TERMUX_EXEC[@]%$(TERMUX_ENV__S_TERMUX_EXEC)%g' \
		 -e 's%[@]TERMUX_ENV__SE_TERMUX_EXEC[@]%$(TERMUX_ENV__SE_TERMUX_EXEC)%g' \
		 -e 's%[@]TERMUX_ENV__S_TERMUX_EXEC_TESTS[@]%$(TERMUX_ENV__S_TERMUX_EXEC_TESTS)%g' \
		 -e 's%[@]TERMUX_ENV__SE_TERMUX_EXEC_TESTS[@]%$(TERMUX_ENV__SE_TERMUX_EXEC_TESTS)%g' \
		 -e 's%[@]TERMUX_APP__NAMESPACE[@]%$(TERMUX_APP__NAMESPACE)%g' \
		 -e 's%[@]TERMUX_APP__SHELL_ACTIVITY__COMPONENT_NAME[@]%$(TERMUX_APP__SHELL_ACTIVITY__COMPONENT_NAME)%g' \
		 -e 's%[@]TERMUX_APP__SHELL_SERVICE__COMPONENT_NAME[@]%$(TERMUX_APP__SHELL_SERVICE__COMPONENT_NAME)%g' \
		 < "$1.in" > "$1"
endef

define make-executable
	@chmod u+x "$1"
endef



all: runtime-binary-tests
	@echo "Building lib/libtermux-exec.so"
	@mkdir -p build/lib
	$(CC) $(CFLAGS) $(LDFLAGS) src/termux-exec.c $(C_SOURCE) -shared -fPIC \
		-fvisibility=hidden \
		$(TERMUX_CONSTANTS) \
		-o build/lib/libtermux-exec.so


	@echo "Building tests/termux-exec-tests"
	$(call replace-termux-constants,tests/termux-exec-tests)
	$(call make-executable,tests/termux-exec-tests)

	@echo "Building tests/runtime-script-tests"
	$(call replace-termux-constants,tests/runtime-script-tests)
	$(call make-executable,tests/runtime-script-tests)


	@echo "Building tests/unit-tests"
	@mkdir -p build/tests
	$(CC) $(CFLAGS) $(LDFLAGS) -g tests/unit-tests.c $(C_SOURCE) \
		$(FSANTIZE_FLAGS) \
		$(TERMUX_CONSTANTS) \
		-DTERMUX_EXEC__RUNNING_TESTS=1 \
		-o build/tests/unit-tests-fsanitize
	$(CC) $(CFLAGS) $(LDFLAGS) -g tests/unit-tests.c $(C_SOURCE) \
		$(TERMUX_CONSTANTS) \
		-DTERMUX_EXEC__RUNNING_TESTS=1 \
		-o build/tests/unit-tests-nofsanitize


	@mkdir -p build/tests/files


	@echo "Building tests/files/exec/*"
	@mkdir -p build/tests/files/exec
	$(CC) $(CFLAGS) $(LDFLAGS) tests/files/exec/print-args-binary.c \
		-o build/tests/files/exec/print-args-binary
	$(call make-executable,tests/files/exec/print-args-linux-script.sh)

	$(call replace-termux-constants,tests/files/exec/print-args-termux-script.sh)
	$(call make-executable,tests/files/exec/print-args-termux-script.sh)


	$(call replace-termux-constants,termux-exec-package.json)

runtime-binary-tests:
	@echo "Building tests/runtime-binary-tests$(TERMUX_EXEC_TESTS_API_LEVEL)"
	@mkdir -p build/tests
	$(CC) $(CFLAGS) $(LDFLAGS) -g tests/runtime-binary-tests.c $(C_SOURCE) \
		$(FSANTIZE_FLAGS) \
		$(TERMUX_CONSTANTS) \
		-o build/tests/runtime-binary-tests-fsanitize$(TERMUX_EXEC_TESTS_API_LEVEL)
	$(CC) $(CFLAGS) $(LDFLAGS) -g tests/runtime-binary-tests.c $(C_SOURCE) \
		$(TERMUX_CONSTANTS) \
		-o build/tests/runtime-binary-tests-nofsanitize$(TERMUX_EXEC_TESTS_API_LEVEL)



clean:
	rm -rf build
	rm -f tests/termux-exec-tests
	rm -f tests/runtime-script-tests
	rm -f tests/files/exec/print-args-termux-script.sh
	rm -f termux-exec-package.json

install:
	install build/lib/libtermux-exec.so $(DESTDIR)$(PREFIX)/lib/libtermux-exec.so


	install -d $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec
	install tests/termux-exec-tests $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec/termux-exec-tests
	install tests/runtime-script-tests $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec/runtime-script-tests


	install $(shell find build/tests -maxdepth 1 -type f) -t $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec


	install -d $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec/files


	install -d $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec/files/exec
	install $(shell find build/tests/files/exec -maxdepth 1 -type f) -t $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec/files/exec
	find tests/files/exec -maxdepth 1 \( -name '*.sh' -o -name '*.sym' \) -exec cp -a "{}" $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec/files/exec \;

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libtermux-exec.so
	rm -rf $(DESTDIR)$(PREFIX)/libexec/installed-tests/termux-exec



deb: all
	termux-create-package termux-exec-package.json



test: all
	tests/termux-exec-tests --ld-preload="${CURDIR}/build/lib/libtermux-exec.so" -vvv all

test-runtime: all
	tests/termux-exec-tests --ld-preload="${CURDIR}/build/lib/libtermux-exec.so" -vvv runtime

test-unit: all
	tests/termux-exec-tests --ld-preload="${CURDIR}/build/lib/libtermux-exec.so" -vvv unit



format:
	$(CLANG_FORMAT) -i src/termux-exec.c $(C_SOURCE)
check:
	$(CLANG_FORMAT) --dry-run src/termux-exec.c $(C_SOURCE)
	$(CLANG_TIDY) -warnings-as-errors='*' src/termux-exec.c $(C_SOURCE) -- \
		$(TERMUX_CONSTANTS)



.PHONY: all clean install uninstall deb test test-runtime test-unit format check

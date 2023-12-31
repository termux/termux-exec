#define _GNU_SOURCE
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>

#include "exec/exec.h"
#include "exec/exec_variants.h"
#include "logger/logger.h"
#include "termux/termux_env.h"

/**
 * This file defines methods intercepted by `libtermux-exec.so` using `LD_PRELOAD`.
 *
 * All exported methods must explicitly enable `default` visibility
 * with `__attribute__((visibility("default")))` as `libtermux-exec.so`
 * is compiled with `-fvisibility=hidden` so that no other internal
 * methods get exported.
 *
 * You can check exported symbols for dynamic linking after building with:
 * `nm --demangle --dynamic --defined-only --extern-only /home/builder/.termux-build/termux-exec/src/build/lib/libtermux-exec.so`.
 */

static void init();
static void initLogger();

static bool INIT_LOGGER = true;



__attribute__((visibility("default")))
int execl(const char *name, const char *arg, ...) {
    init();

    va_list ap;
    va_start(ap, arg);
    int result = execl_hook(true, ExecL, name, arg, ap);
    va_end(ap);
    return result;
}

__attribute__((visibility("default")))
int execlp(const char *name, const char *arg, ...) {
    init();

    va_list ap;
    va_start(ap, arg);
    int result = execl_hook(true, ExecLP, name, arg, ap);
    va_end(ap);
    return result;
}

__attribute__((visibility("default")))
int execle(const char *name, const char *arg, ...) {
    init();

    va_list ap;
    va_start(ap, arg);
    int result = execl_hook(true, ExecLE, name, arg, ap);
    va_end(ap);
    return result;
}

__attribute__((visibility("default")))
int execv(const char *name, char *const *argv) {
    init();

    return execv_hook(true, name, argv);
}

__attribute__((visibility("default")))
int execvp(const char *name, char *const *argv) {
    init();

    return execvp_hook(true, name, argv);
}

__attribute__((visibility("default")))
int execvpe(const char *name, char *const *argv, char *const *envp) {
    init();

    return execvpe_hook(true, name, argv, envp);
}

__attribute__((visibility("default")))
int fexecve(int fd, char *const *argv, char *const *envp) {
    init();

    return fexecve_hook(true, fd, argv, envp);
}

__attribute__((visibility("default")))
int execve(const char *name, char *const argv[], char *const envp[]) {
    init();

    return execve_hook(true, name, argv, envp);
}



static void init() {
    // Sometimes when a process starts, errno is set to values like
    // EINVAL (22) and (ECHILD 10). Noticed on Android 11 (aarch64) and
    // Android 14 (x86_64).
    // It is not being caused by termux-exec as it happens even
    // without `LD_PRELOAD` being set.
    // Moreover, errno is 0 before `execve_syscall()` is called by
    // `execve_intercept()` to replace the process, but in the `main()`
    // of new process, errno is no zero, so something happens during
    // the `syscall()` itself or in callers of `main()`. And manually
    // setting errno before `execve_syscall()` does not transfer it
    // to `main()` of new process.
    // Programs should set errno to `0` at init themselves.
    // We unset it here since programs should have already handled their
    // errno if it was set by something else and termux-exec library
    // also has checks to error out if errno is set in various places,
    // like initially in `string_to_int()` called by `get_termux_exec_log_level()`.
    // Saving errno is useless as it will not be transferred anyways.
    // - https://wiki.sei.cmu.edu/confluence/display/c/ERR30-C.+Take+care+when+reading+errno
    errno = 0;

    initLogger();
}

static void initLogger() {
    if (INIT_LOGGER) {
        setDefaultLogTagAndPrefix(TERMUX__NAME);
        setCurrentLogLevel(get_termux_exec_log_level());
        INIT_LOGGER = false;

        logErrorVVerbose("", "TERMUX_EXEC__VERSION: '%s'", TERMUX_EXEC__VERSION);
    }
}

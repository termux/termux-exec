#ifndef EXEC_VARIANTS_H
#define EXEC_VARIANTS_H

#include <stdbool.h>

/**
 * Hooks for all the `exec()` family of functions in `unistd.h`, other
 * than `execve()`, whose hook `execve_hook()` is declared in `exec.h`.
 *
 * - https://man7.org/linux/man-pages/man3/exec.3.html
 *
 * These `exec()` variants end up calling `execve()` and are ported
 * from `libc/bionic/exec.cpp`.
 *
 * For Android `< 14` intercepting `execve()` was enough.
 * For Android `>= 14` requires intercepting the entire `exec()` family
 * of functions. It might be related to the `3031a7e4` commit in `bionic`,
 * in which `exec.cpp` added `memtag-stack` for `execve()` and shifted
 * to calling `__execve()` internally in it.
 *
 * Hooking the entire family should also solve some issues on older
 * Android versions, check `libc/bionic/exec.cpp` git history.
 *
 * Tests for each `exec` family is done by `run_all_exec_wrappers_test`
 * in `runtime-binary-tests.c`.
 *
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/bionic/exec.cpp
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/SYSCALLS.TXT;l=68
 * - https://cs.android.com/android/_/android/platform/bionic/+/3031a7e45eb992d466610bec3bb589c41b74992b
 */


enum EXEC_VARIANTS { ExecVE, ExecL, ExecLP, ExecLE, ExecV, ExecVP, ExecVPE, FExecVE };

static const char * const EXEC_VARIANTS_STR[] = {
    [ExecVE]  = "execve",
    [ExecL]   = "execl", [ExecLP] = "execlp", [ExecLE]  = "execle",
    [ExecV]   = "execv", [ExecVP] = "execvp", [ExecVPE] = "execvpe",
    [FExecVE] = "fexecve"
};


/**
 * Hook for the `execl()`, `execle()` and `execlp()` methods in `unistd.h`
 * which redirects the call to `execve()`.
 */
int execl_hook(bool hook, int variant, const char *name, const char *argv0, va_list ap);

/**
 * Hook for the `execv()` method in `unistd.h` which redirects the call to `execve()`.
 */
int execv_hook(bool hook, const char *name, char *const *argv);

/**
 * Hook for the `execvp()` method in `unistd.h` which redirects the call to `execve()`.
 */
int execvp_hook(bool hook, const char *name, char *const *argv);

/**
 * Hook for the `execvpe()` method in `unistd.h` which redirects the call to `execve()`.
 */
int execvpe_hook(bool hook, const char *name, char *const *argv, char *const *envp);

/**
 * Hook for the `fexecve()` method in `unistd.h` which redirects the call to `execve()`.
 */
int fexecve_hook(bool hook, int fd, char *const *argv, char *const *envp);

#endif // EXEC_VARIANTS_H

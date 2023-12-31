/*-
 * Copyright (c) 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <paths.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../exec/exec.h"
#include "../logger/logger.h"
#include "exec_variants.h"

static const char* LOG_TAG = "exec";

int execl_hook(bool hook, int variant, const char *name, const char *argv0, va_list ap) {
    if (hook) {
        logErrorDebug(LOG_TAG, "<----- %s() hooked ----->", EXEC_VARIANTS_STR[variant]);
    }

    // Count the arguments.
    va_list count_ap;
    va_copy(count_ap, ap);
    size_t n = 1;
    while (va_arg(count_ap, char *) != NULL) {
        ++n;
    }
    va_end(count_ap);

    // Construct the new argv.
    char *argv[n + 1];
    argv[0] = (char *)argv0;
    n = 1;
    while ((argv[n] = va_arg(ap, char *)) != NULL) {
        ++n;
    }

    // Collect the argp too.
    char **argp = (variant == ExecLE) ? va_arg(ap, char **) : environ;

    va_end(ap);

    return (variant == ExecLP) ? execvp_hook(false, name, argv) : execve_hook(false, name, argv, argp);
}


int execv_hook(bool hook, const char *name, char *const *argv) {
    if (hook) {
        logErrorDebug(LOG_TAG, "<----- execv() hooked ----->");
    }

    return execve_hook(false, name, argv, environ);
}

int execvp_hook(bool hook, const char *name, char *const *argv) {
    if (hook) {
        logErrorDebug(LOG_TAG, "<----- execvp() hooked ----->");
    }

    return execvpe_hook(false, name, argv, environ);
}

int __exec_as_script(const char *buf, char *const *argv, char *const *envp) {
    size_t arg_count = 1;
    while (argv[arg_count] != NULL)
        ++arg_count;

    const char *script_argv[arg_count + 2];
    script_argv[0] = "sh";
    script_argv[1] = buf;
    memcpy(script_argv + 2, argv + 1, arg_count * sizeof(char *));
    return execve_hook(false, _PATH_BSHELL, (char **const)script_argv, envp);
}

int execvpe_hook(bool hook, const char *name, char *const *argv, char *const *envp) {
    if (hook) {
        logErrorDebug(LOG_TAG, "<----- execvpe() hooked ----->");
    }

    // Do not allow null name.
    if (name == NULL || *name == '\0') {
        errno = ENOENT;
        return -1;
    }

    // If it's an absolute or relative path name, it's easy.
    if (strchr(name, '/') && execve_hook(false, name, argv, envp) == -1) {
        if (errno == ENOEXEC)
            return __exec_as_script(name, argv, envp);
        return -1;
    }

    // Get the path we're searching.
    const char *path = getenv("PATH");
    if (path == NULL)
        path = _PATH_DEFPATH;

    // Make a writable copy.
    size_t len = strlen(path) + 1;
    char writable_path[len];
    memcpy(writable_path, path, len);

    bool saw_EACCES = false;

    // Try each element of $PATH in turn...
    char *strsep_buf = writable_path;
    const char *dir;
    while ((dir = strsep(&strsep_buf, ":"))) {
        // It's a shell path: double, leading and trailing colons
        // mean the current directory.
        if (*dir == '\0')
            dir = ".";

        size_t dir_len = strlen(dir);
        size_t name_len = strlen(name);

        char buf[dir_len + 1 + name_len + 1];
        mempcpy(mempcpy(mempcpy(buf, dir, dir_len), "/", 1), name, name_len + 1);

        execve_hook(false, buf, argv, envp);
        switch (errno) {
        case EISDIR:
        case ELOOP:
        case ENAMETOOLONG:
        case ENOENT:
        case ENOTDIR:
            break;
        case ENOEXEC:
            return __exec_as_script(buf, argv, envp);
            return -1;
        case EACCES:
            saw_EACCES = true;
            break;
        default:
            return -1;
        }
    }
    if (saw_EACCES)
        errno = EACCES;
    return -1;
}

int fexecve_hook(bool hook, int fd, char *const *argv, char *const *envp) {
    if (hook) {
        logErrorDebug(LOG_TAG, "<----- fexecve() hooked ----->");
    }

    char buf[40];
    snprintf(buf, sizeof(buf), "/proc/self/fd/%d", fd);
    execve_hook(false, buf, argv, envp);
    if (errno == ENOENT)
        errno = EBADF;
    return -1;
}

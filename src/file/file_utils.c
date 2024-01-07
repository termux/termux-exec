#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../logger/logger.h"
#include "file_utils.h"

static const char* LOG_TAG = "file_utils";

#include "canonicalize.c"

char* absolutize_path(const char *path, char *absolute_path, int buffer_len) {
    if (buffer_len < PATH_MAX) {
        errno = EINVAL;
        return NULL;
    }

    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }

    size_t path_len = strlen(path);
    if (path_len < 1) {
        errno = EINVAL;
        return NULL;
    }

    if (path_len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    if (path[0] != '/') {
        char pwd[PATH_MAX];
        if (getcwd(pwd, sizeof(pwd)) == NULL) {
            return NULL;
        }

        size_t pwd_len = strlen(pwd);
        // Following a change in Linux 2.6.36, the pathname returned by the
        // getcwd() system call will be prefixed with the string
        // "(unreachable)" if the current directory is not below the root
        // directory of the current process (e.g., because the process set a
        // new filesystem root using chroot(2) without changing its current
        // directory into the new root).  Such behavior can also be caused
        // by an unprivileged user by changing the current directory into
        // another mount namespace.  When dealing with pathname from
        // untrusted sources, callers of the functions described in this
        // page should consider checking whether the returned pathname
        // starts with '/' or '(' to avoid misinterpreting an unreachable
        // path as a relative pathname.
        // - https://man7.org/linux/man-pages/man3/getcwd.3.html
        if (pwd_len < 1 || pwd[0] != '/') {
            errno = ENOENT;
            return NULL;
        }

        memcpy(absolute_path, pwd, pwd_len);
        if (absolute_path[pwd_len - 1] != '/') {
            absolute_path[pwd_len] = '/';
            pwd_len++;
        }

        size_t absolutized_path_len = pwd_len + path_len;
        if (absolutized_path_len >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return NULL;
        }

        memcpy(absolute_path + pwd_len, path, path_len);
        absolute_path[absolutized_path_len] = '\0';
    } else {
        strcpy(absolute_path, path);
    }

    return absolute_path;
}





char* normalize_path(char* path, bool keepEndSeparator, bool removeDoubleDot) {
    return __normalizePath(path, keepEndSeparator, removeDoubleDot);
}




char* remove_dup_separator(char* path, bool keepEndSeparator) {
    if (path == NULL || *path == '\0') {
        return NULL;
    }

    char* in = path;
    char* out = path;
    char prevChar = 0;
    int n = 0;
    for (; *in != '\0'; in++) {
        // Remove duplicate path separators
        if (!(*in == '/' && prevChar == '/')) {
            *(out++) = *in;
            n++;
        }
        prevChar = *in;
    }
    *out = '\0';

    // Remove the trailing path separator, except when path equals `/`
    if (!keepEndSeparator && prevChar == '/' && n > 1) {
        *(--out) = '\0';
    }

    return path;
}





bool is_path_in_dir_path(const char* label, const char* path, const char* dir_path, bool ensure_under) {
    if (path == NULL || *path != '/' || dir_path == NULL || *dir_path != '/' ) {
        return 1;
    }

    // If root "/", preserve it as is, otherwise append "/" to dir_path
    char *dir_sub_path;
    bool is_rootfs = strcmp(dir_path, "/") == 0;
    if (asprintf(&dir_sub_path, is_rootfs ? "%s" : "%s/", dir_path) == -1) {
        errno = ENOMEM;
        logStrerrorDebug(LOG_TAG, "asprintf failed for while checking if the path '%s' is under %s '%s'", path, label, dir_path);
        return -1;
    }

    int result;
    if (ensure_under) {
        result = strcmp(dir_sub_path, path) != 0 && string_starts_with(path, dir_sub_path) ? 0 : 1;
    } else {
        result = strcmp(dir_sub_path, path) == 0 || string_starts_with(path, dir_sub_path) ? 0 : 1;
    }

    free(dir_sub_path);

    return result;
}

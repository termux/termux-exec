#define _GNU_SOURCE
#include <errno.h>
#include <libgen.h>
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/limits.h>

#include "../data/data_utils.h"
#include "../file/file_utils.h"
#include "../logger/logger.h"
#include "termux_env.h"
#include "termux_files.h"

static const char* LOG_TAG = "termux_files";

int get_termux_rootfs_dir_from_env(const char* log_tag, char *buffer, size_t buffer_len) {
    (void)LOG_TAG;
    (void)log_tag;

    const char* rootfs_dir = getenv(ENV__TERMUX__ROOTFS);
    if (rootfs_dir != NULL) {
        size_t rootfs_dir_len = strlen(rootfs_dir);
        if (rootfs_dir_len > 0) {
            if (rootfs_dir[0] == '/' && rootfs_dir_len < TERMUX__ROOTFS_DIR_MAX_LEN) {
                if (buffer_len <= rootfs_dir_len) {
                    #ifndef TERMUX_EXEC__RUNNING_TESTS
                    logErrorDebug(LOG_TAG, "The rootfs_dir '%s' with length '%zu' too long to fit in the buffer with length '%zu'",
                        rootfs_dir, rootfs_dir_len, buffer_len);
                    #endif
                    errno = EINVAL;
                    return -1;
                }

                strcpy(buffer, rootfs_dir);
                return 0;
            }

            #ifndef TERMUX_EXEC__RUNNING_TESTS
            logErrorVVerbose(log_tag, "Ignoring invalid rootfs_dir with length '%zu' set in '%s' env variable: '%s'",
            rootfs_dir_len, ENV__TERMUX__ROOTFS, rootfs_dir);
            logErrorVVerbose(log_tag, "The rootfs_dir must be an absolute path starting with a '/' with max length '%d' including the null '\0' terminator",
            TERMUX__ROOTFS_DIR_MAX_LEN);
            #endif
        }
    }

    return 1;
}

char const* get_termux_rootfs_dir_from_env_or_default(const char* log_tag, char *buffer, size_t buffer_len) {
    const char *rootfs_dir;
    int result = get_termux_rootfs_dir_from_env(log_tag, buffer, buffer_len);
    if (result < 0) {
        return NULL;
    } else if (result == 0) {
        normalize_path(buffer, false, true);
        rootfs_dir = buffer;
        logErrorVVerbose(log_tag, "rootfs_dir: '%s'", rootfs_dir);
        return rootfs_dir;
    }

    // Use default rootfs_dir provided at build time by Makefile
    size_t rootfs_dir_len = strlen(TERMUX__ROOTFS);
    if (TERMUX__ROOTFS[0] != '/' || buffer_len <= rootfs_dir_len) {
        logErrorDebug(log_tag, "The default rootfs_dir '%s' with length '%zu' must be an absolute path starting with a '/' with max length '%zu'",
            TERMUX__ROOTFS, rootfs_dir_len, buffer_len);
        errno = EINVAL;
        return NULL;
    }

    strcpy(buffer, TERMUX__ROOTFS);
    normalize_path(buffer, false, true);
    rootfs_dir = TERMUX__ROOTFS;
    logErrorVVerbose(log_tag, "default_rootfs_dir: '%s'", rootfs_dir);

    // In case termux-exec is compiled for rootfs_dir of a different package
    if (access(rootfs_dir, (R_OK | X_OK)) != 0) {
        logStrerrorDebug(log_tag, "The default rootfs_dir '%s' is not readable or executable", TERMUX__ROOTFS);
        return NULL;
    }

    return rootfs_dir;
}



char *termux_prefix_path(const char* log_tag, const char *termux_rootfs_dir, const char *executable_path,
    char *buffer, size_t buffer_len) {
    (void)log_tag;

    size_t executable_path_len = strlen(executable_path);
    if (buffer_len <= executable_path_len) {
        #ifndef TERMUX_EXEC__RUNNING_TESTS
        logErrorDebug(LOG_TAG, "The original executable_path '%s' with length '%zu' to prefix is too long to fit in the buffer with length '%zu'",
            executable_path, executable_path_len, buffer_len);
        #endif
        errno = ENAMETOOLONG;
        return NULL;
    }

    // If termux_rootfs_dir equals `/` or executable_path is not an absolute path
    if ((strlen(termux_rootfs_dir) == 1 && termux_rootfs_dir[0] == '/') ||
        executable_path[0] != '/') {
        strcpy(buffer, executable_path);
        return buffer;
    }

    char termux_bin_path[TERMUX__BIN_DIR_MAX_LEN + 1];
    bool termux_rootfs_is_system = strcmp(termux_rootfs_dir, "/system") == 0;
    const char* bin_fmt;

    // If executable_path equals with `/bin` or `/usr/bin` (currently not `/xxx/bin`)
    if (strcmp(executable_path, "/bin") == 0 || strcmp(executable_path, "/usr/bin") == 0) {
        bin_fmt = termux_rootfs_is_system ? "%s/bin" : "%s/usr/bin";
        snprintf(termux_bin_path, sizeof(termux_bin_path), bin_fmt, termux_rootfs_dir);
        strcpy(buffer, termux_bin_path);
        return buffer;
    }

    char *bin_match = strstr(executable_path, "/bin/");
    // If executable_path starts with `/bin/` or `/xxx/bin`
    if (bin_match == executable_path || bin_match == (executable_path + 4)) {

        bin_fmt = termux_rootfs_is_system ? "%s/bin/" : "%s/usr/bin/";
        int termux_bin_path_len = snprintf(termux_bin_path, sizeof(termux_bin_path),
            bin_fmt, termux_rootfs_dir);

        strcpy(buffer, termux_bin_path);
        char *dest = buffer + termux_bin_path_len;
        // Copy what comes after `/bin/`:
        const char *src = bin_match + 5;
        size_t prefixed_path_len = termux_bin_path_len + strlen(src);
        if (buffer_len <= prefixed_path_len) {
            #ifndef TERMUX_EXEC__RUNNING_TESTS
            logErrorDebug(log_tag, "The prefixed_path '%s%s' with length '%zu' is too long to fit in the buffer with length '%zu'",
                termux_bin_path, src, prefixed_path_len, buffer_len);
            #endif
            errno = ENAMETOOLONG;
            return NULL;
        }

        strcpy(dest, src);
        return buffer;
    } else {
        strcpy(buffer, executable_path);
        return buffer;
    }
}



char* get_fd_realpath(const char* log_tag, const char *path, char *real_path, size_t buffer_len) {
    char path_string[strlen(path) + 1];
    strncpy(path_string, path, sizeof(path_string));
    char* fd_string = basename(path_string);

    int fd = string_to_int(fd_string, -1, log_tag, "Failed to convert fd string '%s' to fd for fd path '%s'", fd_string, path);
    if (fd < 0) {
        return NULL;
    }

    #ifndef TERMUX_EXEC__RUNNING_TESTS
    logErrorVVerbose(log_tag, "fd_path: '%s', fd: '%d'", path, fd);
    #endif

    struct stat fd_statbuf;
    int fd_result = fstat(fd, &fd_statbuf);
    if (fd_result < 0) {
        logStrerrorDebug(log_tag, "Failed to stat fd '%d' for fd path '%s'", fd, path);
        return NULL;
    }

    ssize_t length = readlink(path, real_path, buffer_len - 1);
    if (length < 0) {
        logStrerrorDebug(log_tag, "Failed to get real path for fd path '%s'", path);
        return NULL;
    } else {
        real_path[length] = '\0';
    }

    #ifndef TERMUX_EXEC__RUNNING_TESTS
    logErrorVVerbose(log_tag, "real_path: '%s'", real_path);
    #endif

    // Check if fd is for a regular file.
    // We should log real path first before doing this check.
    if (!S_ISREG(fd_statbuf.st_mode)) {
        #ifndef TERMUX_EXEC__RUNNING_TESTS
        logErrorDebug(log_tag, "The real path '%s' for fd path '%s' is of type '%d' instead of a regular file",
        real_path, path, fd_statbuf.st_mode & S_IFMT);
        #endif
        errno = ENOEXEC;
        return NULL;
    }

    size_t real_path_length = strlen(real_path);
    if (real_path_length < 1 || real_path[0] != '/') {
        logErrorDebug(log_tag, "A non absolute real path '%s' returned for fd path '%s'", real_path, path);
        errno = ENOEXEC;
        return NULL;
    }

    struct stat path_statbuf;
    int path_result = stat(real_path, &path_statbuf);
    if (path_result < 0) {
        logStrerrorDebug(log_tag, "Failed to stat real path '%s' returned for fd path '%s'", real_path, path);
        return NULL;
    }

    // If the original file when fd was opened has now been replaced with a different file.
    if (fd_statbuf.st_dev != path_statbuf.st_dev || fd_statbuf.st_ino != path_statbuf.st_ino) {
        logErrorDebug(log_tag, "The file at real path '%s' is not for the original fd '%d'", real_path, fd);
        errno = ENXIO;
        return NULL;
    }

    return real_path;
}



int is_path_under_termux_rootfs_dir(const char* log_tag, const char *path,
    const char *termux_rootfs_dir) {
    if (path == NULL || *path == '\0') {
        return 1;
    }

    if (strstr(path, "/fd/") != NULL && regex_match(path, REGEX__PROC_FD_PATH, REG_EXTENDED) == 0) {
        char real_path_buffer[PATH_MAX];
        (void)real_path_buffer;
        char *real_path = get_fd_realpath(log_tag, path, real_path_buffer, sizeof(real_path_buffer));
        if (real_path == NULL) {
            return -1;
        }

        return is_path_in_dir_path("termux_rootfs_dir", real_path, termux_rootfs_dir, true);
    } else {
        return is_path_in_dir_path("termux_rootfs_dir", path, termux_rootfs_dir, true);
    }
}

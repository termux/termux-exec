#ifndef TERMUX_FILES_H
#define TERMUX_FILES_H

#include <stdbool.h>

/**
 * The max length for the `TERMUX__ROOTFS` directory including the null '\0' terminator.
 *
 * Check https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#file-path-limits
 * for why the value `86` is chosen.
 *
 * Default value: `86`
 */
#define TERMUX__ROOTFS_DIR_MAX_LEN 86

/**
 * The max length for the `TERMUX__PREFIX` directory including the null '\0' terminator.
 *
 * Default value: `90`
 */
#define TERMUX__PREFIX_DIR_MAX_LEN (TERMUX__ROOTFS_DIR_MAX_LEN + 4) // "/usr" (90)

/**
 * The max length for the `TERMUX__BIN_DIR` including the null '\0' terminator.
 *
 * Default value: `94`
 */
#define TERMUX__BIN_DIR_MAX_LEN (TERMUX__PREFIX_DIR_MAX_LEN + 4) // "/bin" (94)

/**
 * The max safe length for a sub file path under the `TERMUX__BIN_DIR` including the null '\0' terminator.
 *
 * This allows for a filename with max length `32` so that the path length
 * is under `128` (`BINPRM_BUF_SIZE`) for Linux kernel `< 5.1`.
 * Check `exec.h` docs.
 *
 * Default value: `127`
 */
#define TERMUX__BIN_FILE_SAFE_MAX_LEN (TERMUX__BIN_DIR_MAX_LEN + 34) // "/<filename_with_len_32>" (127)



/**
 * Path to `/proc/self/fd/<num>` , `/proc/<pid>/fd/<num>`.
 */
#define REGEX__PROC_FD_PATH "^((/proc/(self|[0-9]+))|(/dev))/fd/[0-9]+$"



/**
 * Get the termux rootfs directory from the `ENV__TERMUX__ROOTFS` env
 * variable value if its set to a valid  absolute path with max length
 * `TERMUX__ROOTFS_DIR_MAX_LEN`.
 *
 *
 * @return Returns `0` if a valid rootfs directory is found and copied
 * to the buffer, `1` if valid rootfs directory is not found, otherwise
 * `-1` on other failures.
 */
int get_termux_rootfs_dir_from_env(const char* log_tag, char *buffer, size_t buffer_len);

/**
 * Get the termux rootfs directory from the `ENV__TERMUX__ROOTFS` env
 * variable by calling `get_termux_rootfs_dir_from_env()`, otherwise
 * if it fails, then return `TERMUX__ROOTFS` set by `Makefile` if
 * it is readable and executable.
 *
 * @return Returns the `char *` to rootfs_dir on success, otherwise `NULL`.
 */
char const* get_termux_rootfs_dir_from_env_or_default(const char* log_tag, char *buffer, size_t buffer_len);



/**
 * Prefix `bin` directory paths like `/bin` and `/usr/bin` with
 * termux rootfs directory, like convert `/bin/sh` to `<termux_rootfs_dir>/bin/sh`.
 *
 * The buffer size must at least be `TERMUX__ROOTFS_DIR_MAX_LEN` + strlen(executable_path)`,
 * or preferably `PATH_MAX`.
 *
 * @return Returns the `char *` to original or prefixed directory on
 * success, otherwise `NULL` on failures.
 */
char *termux_prefix_path(const char* log_tag, const char *termux_rootfs_dir,
    const char *executable_path, char *buffer, size_t buffer_len);



/**
 * Get the real path of an fd `path`.
 *
 * **The `path` passed must match `REGEX__PROC_FD_PATH`, as no checks
 * are done by this method for whether path is for an fd path. If
 * an integer is set to the path basename, then it is assumed to be the
 * fd number, even if it is for a valid fd to be checked or not.**
 *
 * - https://stackoverflow.com/questions/1188757/retrieve-filename-from-file-descriptor-in-c
 */
char* get_fd_realpath(const char* log_tag, const char *path, char *real_path, size_t buffer_len);



/**
 * Check whether the `path` is in `termux_rootfs_dir`. If path is
 * an fd path matched by `REGEX__PROC_FD_PATH`, then the real path
 * of the fd returned by `get_fd_realpath()` will be checked instead.
 *
 * **Both `path` and `termux_rootfs_dir` must be normalized paths, as
 * `is_path_in_dir_path()` called by this method will currently
 * not normalize either path by itself.**
 *
 * @param log_tag The log tag to use for logging.
 * @param path The `path` to check.
 * @param termux_rootfs_dir The **normalized** path to termux rootfs
 *                          directory returned by
 *                          `get_termux_rootfs_dir_from_env_or_default()`.
 * @return Returns `0` if `path` is in `termux_rootfs_dir`, `1` if `path` is not in `termux_rootfs_dir`,
 *         otherwise `-1` on other failures.
 */
int is_path_under_termux_rootfs_dir(const char* log_tag, const char *path, const char *termux_rootfs_dir);

#endif // TERMUX_FILES_H

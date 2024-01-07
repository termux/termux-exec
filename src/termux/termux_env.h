#ifndef TERMUX_ENV_H
#define TERMUX_ENV_H

#include <stdbool.h>

/*
 * Environment for Unix-like systems.
 *
 * - https://manpages.debian.org/testing/manpages/environ.7.en.html
 * - https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html
 */

/** Environment variable for the list of directory path prefixes
 * separated with colons ":" that certain functions and utilities
 * apply in searching for an executable file known only by a filename.
 *
 * - https://manpages.debian.org/testing/manpages/environ.7.en.html#PATH
 *
 * Type: `string`
 */
#define ENV__PATH "PATH"
#define ENV_PREFIX__PATH ENV__PATH "="

/**
 * Environment variable for the list of directory paths
 * separated with colons ":" that should be searched in for dynamic
 * shared libraries to link programs against.
 *
 * - https://manpages.debian.org/testing/manpages/ld.so.8.en.html#LD_LIBRARY_PATH
 *
 * Type: `string`
 */
#define ENV__LD_LIBRARY_PATH "LD_LIBRARY_PATH"
#define ENV_PREFIX__LD_LIBRARY_PATH ENV__LD_LIBRARY_PATH "="

/**
 * Environment variable for the list of ELF shared object paths separated with colons ":" to be
 * loaded before all others. This feature can be used to selectively override functions in other
 * shared objects.
 *
 * - https://manpages.debian.org/testing/manpages/ld.so.8.en.html#LD_PRELOAD
 *
 * Type: `string`
 */
#define ENV__LD_PRELOAD "LD_PRELOAD"
#define ENV_PREFIX__LD_PRELOAD ENV__LD_PRELOAD "="



/*
 * Environment for Termux.
 */

/**
 * Environment variable for the Termux rootfs directory path.
 *
 * Type: `string`
 * Default key: `TERMUX__ROOTFS`
 * Default value: null
 * Values:
 * - An absolute path with max length `TERMUX__ROOTFS_DIR_MAX_LEN` (`85`) including the null `\0` terminator.
 */
#define ENV__TERMUX__ROOTFS TERMUX_ENV__S_TERMUX "ROOTFS"

/**
 * Environment variable for the SeLinux process context of the Termux
 * app process and its child processes.
 *
 * Type: `string`
 * Default key: `TERMUX__SE_PROCESS_CONTEXT`
 * Default value: null
 * Values:
 * - A valid Android SeLinux process context that matches `REGEX__PROCESS_CONTEXT`.
 */
#define ENV__TERMUX__SE_PROCESS_CONTEXT TERMUX_ENV__S_TERMUX "SE_PROCESS_CONTEXT"



/*
 * Environment for `termux-exec`.
 */

/**
 * Environment variable for the log level for `termux-exec`.
 *
 * Type: `int`
 * Default key: `TERMUX_EXEC__LOG_LEVEL`
 * Default value: DEFAULT_LOG_LEVEL
 * Values:
 * - `0` (`OFF`) - Log nothing.
 * - `1` (`NORMAL`) - Log error, warn and info messages and stacktraces.
 * - `2` (`DEBUG`) - Log debug messages.
 * - `3` (`VERBOSE`) - Log verbose messages.
 * - `4` (`VVERBOSE`) - Log very verbose messages.
 */
#define ENV__TERMUX_EXEC__LOG_LEVEL TERMUX_ENV__S_TERMUX_EXEC "LOG_LEVEL"



/**
 * Environment variable for whether `termux-exec` should intercept
 * `execve()` wrapper [declared in `unistd.h`.
 *
 * Type: `bool`
 * Default key: `TERMUX_EXEC__INTERCEPT_EXECVE`
 * Default value: E_DEF_VAL__TERMUX_EXEC__INTERCEPT_EXECVE
 * Values:
 * - `true` - Intercept `execve()` enabled.
 * - `false` - Intercept `execve()` disabled.
 */
#define ENV__TERMUX_EXEC__INTERCEPT_EXECVE TERMUX_ENV__S_TERMUX_EXEC "INTERCEPT_EXECVE"
static const bool E_DEF_VAL__TERMUX_EXEC__INTERCEPT_EXECVE = true;

/**
 * Environment variable for whether use System Linker Exec Solution,
 * like to bypass App Data File Execute Restrictions.
 *
 * See also `should_system_linker_exec()`
 *
 * Type: `string`
 * Default key: `TERMUX_EXEC__SYSTEM_LINKER_EXEC`
 * Default value: E_DEF_VAL__TERMUX_EXEC__SYSTEM_LINKER_EXEC
 * Values:
 * - `disable` - The `system_linker_exec` will be disabled.
 * - `enable` - The `system_linker_exec` will be enabled but only if required.
 * - `force` - The `system_linker_exec` will be force enabled even if not required.
 */
#define ENV__TERMUX_EXEC__SYSTEM_LINKER_EXEC TERMUX_ENV__S_TERMUX_EXEC "SYSTEM_LINKER_EXEC"
static const int E_DEF_VAL__TERMUX_EXEC__SYSTEM_LINKER_EXEC = 1;



/**
 * Environment variable for the path to the executable file is being
 * executed by `execve()` is using `system_linker_exec`.
 *
 * Type: `string`
 * Default key: `TERMUX_EXEC__PROC_SELF_EXE`
 * Values:
 * - The normalized, absolutized and prefixed path for the executable
 * file is being executed by `execve()` if `system_linker_exec` is
 * being used.
 */
#define ENV__TERMUX_EXEC__PROC_SELF_EXE TERMUX_ENV__S_TERMUX_EXEC "PROC_SELF_EXE"
#define ENV_PREFIX__TERMUX_EXEC__PROC_SELF_EXE ENV__TERMUX_EXEC__PROC_SELF_EXE "="



/*
 * Environment for `termux-exec-tests`.
 */

/**
 * Environment variable for the log level for `termux-exec-tests`.
 *
 * Type: `int`
 * Default key: `TERMUX_EXEC_TESTS__LOG_LEVEL`
 * Default value: DEFAULT_LOG_LEVEL
 * Values:
 * - `0` (`OFF`) - Log nothing.
 * - `1` (`NORMAL`) - Log error, warn and info messages and stacktraces.
 * - `2` (`DEBUG`) - Log debug messages.
 * - `3` (`VERBOSE`) - Log verbose messages.
 * - `4` (`VVERBOSE`) - Log very verbose messages.
 */
#define ENV__TERMUX_EXEC_TESTS__LOG_LEVEL TERMUX_ENV__S_TERMUX_EXEC_TESTS "LOG_LEVEL"





/**
 * Get an environment variable value as a `bool`.
 *
 * The following values are parsed as `true`: `1`, `true`, `on`, `yes`, `y`
 * The following values are parsed as `false`: `0`, `false`, `off`, `no`, `n`
 * For any other value, the `def` value will be returned.
 * Comparisons are case-sensitive.
 *
 * The purpose of this function is to have a single canonical parser for yes-or-no indications
 * throughout the system.
 *
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:frameworks/base/core/java/android/os/SystemProperties.java;l=198
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:frameworks/base/core/jni/android_os_SystemProperties.cpp;l=123
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/libbase/include/android-base/parsebool.h
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/libbase/parsebool.cpp
 *
 * @param value The value to parse as a `bool`.
 * @param def The default value.
 * @return Return the parsed `bool` value.
 */
bool get_bool_env_value(const char *name, bool def);



/**
 * Check if a env variable in `vars` exists in the `envp` where `n` is
 * number of elements in `vars` to search starting from the first element.
 *
 * Each value in vars must be in the format `<name>=`, like `PATH=`.
 *
 * @return Returns `true` if even a single variable is found, otherwise
 * `false`.
 */
bool are_vars_in_env(char *const *envp, const char *vars[], int n);



/**
 * Returns the `termux-exec` config for logger log level
 * based on the `ENV__TERMUX_EXEC__LOG_LEVEL` env variable.
 * *
 * @return Return the value if `ENV__TERMUX_EXEC__LOG_LEVEL` is
 * set, otherwise defaults to `DEFAULT_LOG_LEVEL`.
 */
int get_termux_exec_log_level();



/**
 * Returns the `termux-exec` config for whether `execve` should be
 * intercepted based on the `ENV__TERMUX_EXEC__INTERCEPT_EXECVE` env variable.
 *
 * See also `get_bool_env_value()`.
 *
 * @return Return `true` if `ENV__TERMUX_EXEC__INTERCEPT_EXECVE` is
 * bool enabled, `false` if bool disabled, otherwise defaults to `true`.
 */
bool is_termux_exec_execve_intercept_enabled();

/**
 * Returns the `termux-exec` config for `system_linker_exec` based on
 * the `ENV__TERMUX_EXEC__SYSTEM_LINKER_EXEC` env variable.
 *
 * @return Return `0` if `ENV__TERMUX_EXEC__SYSTEM_LINKER_EXEC` is set
 * to `disable`, `1` if set to `enable`, `2` if set to `force`,
 * otherwise defaults to `1` (`enable`).
 */
int get_termux_exec_system_linker_exec_config();



/**
 * Returns the `termux-exec-tests` config for logger log level
 * based on the `ENV__TERMUX_EXEC_TESTS__LOG_LEVEL` env variable.
 * *
 * @return Return the value if `ENV__TERMUX_EXEC_TESTS__LOG_LEVEL` is
 * set, otherwise defaults to `DEFAULT_LOG_LEVEL`.
 */
int get_termux_exec_tests_log_level();

#endif // TERMUX_ENV_H

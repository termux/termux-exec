#ifndef EXEC_H
#define EXEC_H

#include <stdbool.h>

#include "../termux/termux_env.h"
#include "../termux/termux_files.h"


/** The path to android system linker. */
#if UINTPTR_MAX == 0xffffffff
#define SYSTEM_LINKER_PATH "/system/bin/linker";
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYSTEM_LINKER_PATH "/system/bin/linker64";
#endif





/*
 * For the `execve()` system call, the kernel imposes a maximum length limit on script shebang
 * including the `#!` characters at the start of a script. For Linux kernel `< 5.1`, the limit is `128`
 * characters and for Linux kernel `>= 5.1`, the limit is `256` characters as per `BINPRM_BUF_SIZE`
 * including the null `\0` terminator.
 *
 * If `termux-exec` is set in `LD_PRELOAD` and `TERMUX_EXEC__INTERCEPT_EXECVE` is enabled, then
 * shebang limit is increased to `340` characters defined by `FILE_HEADER__BUFFER_LEN` as shebang
 * is read and script is passed to interpreter as an argument by `termux-exec` manually.
 * Increasing limit to `340` also fixes issues for older Android kernel versions where limit is `128`.
 * The limit is increased to `340`, because `BINPRM_BUF_SIZE` would be set based on the assumption
 * that rootfs is at `/`, so we add Termux rootfs directory max length to it.
 *
 * - https://man7.org/linux/man-pages/man2/execve.2.html
 * - https://en.wikipedia.org/wiki/Shebang_(Unix)#Character_interpretation
 * - https://cs.android.com/android/kernel/superproject/+/0dc2b7de045e6dcfff9e0dfca9c0c8c8b10e1cf3:common/fs/binfmt_script.c;l=34
 * - https://cs.android.com/android/kernel/superproject/+/0dc2b7de045e6dcfff9e0dfca9c0c8c8b10e1cf3:common/include/linux/binfmts.h;l=64
 * - https://cs.android.com/android/kernel/superproject/+/0dc2b7de045e6dcfff9e0dfca9c0c8c8b10e1cf3:common/include/uapi/linux/binfmts.h;l=18
 *
 *
 * The running a script in `bash`, and the interpreter length is `>= 128` (`BINPRM_BUF_SIZE`) and
 * `execve()` system call returns `ENOEXEC` (`Exec format error`), then `bash` will read the file
 * and run it as `bash` shell commands.
 * If interpreter length was `< 128` and `execve()` returned some other error than `ENOEXEC`, then
 * `bash` will try to give a meaningful error.
 * - If script was not executable: `bash: <script_path>: Permission denied`
 * - If script was a directory: `bash: <script_path>: Is a directory`
 * - If `ENOENT` was returned since interpreter file was not found: `bash: <script_path>: cannot execute: required file not found`
 * - If some unhandled errno was returned, like interpreter file was a directory: `bash: <script_path>: <interpreter>: bad interpreter`
 *
 * - https://github.com/bminor/bash/blob/bash-5.2/execute_cmd.c#L5929
 * - https://github.com/bminor/bash/blob/bash-5.2/execute_cmd.c#L5964
 * - https://github.com/bminor/bash/blob/bash-5.2/execute_cmd.c#L5988
 * - https://github.com/bminor/bash/blob/bash-5.2/execute_cmd.c#L6048
 */

/**
 * The max length for entire shebang line for the Linux kernel `>= 5.1` defined by `BINPRM_BUF_SIZE`.
 * Add `FILE_HEADER__` scope to prevent conflicts by header importers.
 *
 * Default value: `256`
 */
#define FILE_HEADER__BINPRM_BUF_SIZE 256

/**
 * The max length for interpreter path in the shebang for `termux-exec`.
 *
 * Default value: `340`
 */
#define FILE_HEADER__INTERPRETER_PATH_MAX_LEN (TERMUX__ROOTFS_DIR_MAX_LEN + FILE_HEADER__BINPRM_BUF_SIZE - 1) // The `- 1` is to only allow one null `\0` terminator.

/**
 * The max length for interpreter arg in the shebang for `termux-exec`.
 *
 * This is same as `FILE_HEADER__BINPRM_BUF_SIZE`.  These is no way to divide `BINPRM_BUF_SIZE`
 * between path and arg, so we give it full buffer size in case it needs it.
 *
 * Default value: `256`
 */
#define FILE_HEADER__INTERPRETER_ARG_MAX_LEN FILE_HEADER__BINPRM_BUF_SIZE

/**
 * The max length for entire shebang line for `termux-exec`.
 *
 * This is same as `FILE_HEADER__INTERPRETER_PATH_MAX_LEN`.
 *
 * Default value: `340`
 */
#define FILE_HEADER__BUFFER_LEN FILE_HEADER__INTERPRETER_PATH_MAX_LEN



/**
 * The info for the file header of an executable file.
 *
 * - https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.eheader.html
 */
struct file_header_info {
    /** Whether executable is an ELF file instead of a script. */
    bool is_elf;

    /**
     * Whether the `Elf32_Ehdr.e_machine != EM_NATIVE`, i.e executable
     * file is 32-bit binary on a 64-bit host.
     */
    bool is_non_native_elf;

    /**
     * The original interpreter path set in the executable file that is
     * not normalized, absolutized or prefixed.
     */
    char const *orig_interpreter_path;

    /**
     * The interpreter path set in the executable file that is
     * normalized, absolutized and prefixed.
     */
    char const *interpreter_path;
    /** The underlying buffer for `interpreter_path`. */
    char interpreter_path_buffer[FILE_HEADER__INTERPRETER_PATH_MAX_LEN];

    /** The arguments to the interpreter set in the executable file. */
    char const *interpreter_arg;
    /** The underlying buffer for `interpreter_arg`. */
    char interpreter_arg_buffer[FILE_HEADER__INTERPRETER_ARG_MAX_LEN];
};



/** The host native architecture. */
#ifdef __aarch64__
#define EM_NATIVE EM_AARCH64
#elif defined(__arm__) || defined(__thumb__)
#define EM_NATIVE EM_ARM
#elif defined(__x86_64__)
#define EM_NATIVE EM_X86_64
#elif defined(__i386__)
#define EM_NATIVE EM_386
#else
#error "unknown arch"
#endif


/** The list of variables that are unset by `modify_exec_env()` if `unset_ld_vars_from_env` is `true`. */
static const char *LD_VARS_TO_UNSET[] __attribute__ ((unused)) = { ENV_PREFIX__LD_LIBRARY_PATH, ENV_PREFIX__LD_PRELOAD };
static int LD_VARS_TO_UNSET_SIZE __attribute__ ((unused)) = 2;


/**
 * Hook for the `execve()` method in `unistd.h`.
 *
 * If `is_termux_exec_execve_intercept_enabled()` returns `true`,
 * then `execve_intercept()` will be called, otherwise
 * `execve_syscall()`.
 *
 * - https://man7.org/linux/man-pages/man3/exec.3.html
 */
int execve_hook(bool hook, const char *executable_path, char *const argv[], char *const envp[]);



/**
 * Inspect file header and set `file_header_info`.
 *
 * @param termux_rootfs_dir The **normalized** path to termux rootfs
 *                          directory returned by
 *                          `get_termux_rootfs_dir_from_env_or_default()`.
 * @param header The file header read from the executable file.
 *               The `FILE_HEADER__BUFFER_LEN` should be used as buffer size
 *               when reading.
 * @param header_len The actual length of the header that was read.
 * @param info The `file_header_info` to set.
 */
int inspect_file_header(char const *termux_rootfs_dir, char *header, size_t header_len,
    struct file_header_info *info);


/**
 * Whether to use `system_linker_exec`, like to bypass app data file
 * execute restrictions.
 *
 * A call is made to `get_termux_exec_system_linker_exec_config()` to
 * get the config for using `system_linker_exec`.
 *
 * If `disable` is set, then `system_linker_exec` should not be used.
 *
 * If `enable` is set, then `system_linker_exec` should only be used if:
 * - `system_linker_exec` is required to bypass app data file execute
 *   restrictions, i.e device is running on Android `>= 10`.
 * - Effective user does not equal root (`0`) and shell (`2000`) user.
 * - `TERMUX__SE_PROCESS_CONTEXT` or its fallback `/proc/self/attr/current`
 *    does not start with `PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_25` and
 *   `PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_27` for which restrictions
 *   are exempted.
 * - `is_path_under_termux_rootfs_dir()` returns `true` for the `executable_path`.
 *
 * If `force` is set, then `system_linker_exec` should only be used if:
 * - `system_linker_exec` is supported, i.e device is running on Android `>= 10`.
 * - `is_path_under_termux_rootfs_dir()` returns `true` for the `executable_path`.
 * This can be used if running in an untrusted app with `targetSdkVersion` `<= 28`.
 *
 * @param executable_path The **normalized** executable or interpreter
 *                        path that will actually be executed.
 * @param termux_rootfs_dir The **normalized** path to termux rootfs
 *                          directory returned by
 *                          `get_termux_rootfs_dir_from_env_or_default()`.
 * @return Returns `0` if `system_linker_exec` should not be used,
 * `1` if `system_linker_exec` should be used, otherwise `-1` on failures.
 */
int should_system_linker_exec(const char *executable_path, const char *termux_rootfs_dir);



/**
 * Whether variables in `LD_VARS_TO_UNSET` should be unset before `exec()`
 * to prevent issues when executing system binaries that are caused
 * if they are set.
 *
 * @param is_non_native_elf The value for `file_header_info.is_non_native_elf`
 *        for the executable file.
 * @param executable_path The **normalized** executable path to check.
 * @return Returns `true` if `is_non_native_elf` equals `true` or
 * `executable_path` starts with `/system/`, but does not equal
 * `/system/bin/sh`, `system/bin/linker` or `/system/bin/linker64`.
 *
 */
bool should_unset_ld_vars_from_env(bool is_non_native_elf, const char *executable_path);

/**
 * Modify the environment for `execve()`.
 *
 * @param envp The current environment pointer.
 * @param new_envp_pointer The new environment pointer to set.
 * @param env_termux_proc_self_exe If set, then it will overwrite or
 *                                 set the `ENV_PREFIX__TERMUX_EXEC__PROC_SELF_EXE`
 *                                 env variable.
 * @param unset_ld_vars_from_env If `true`, then variables in
 *                               `LD_VARS_TO_UNSET` will be unset.
 * @return Returns `0` if successfully modified the env, otherwise
 * `-1` on failures. Its the callers responsibility to call `free()`
 * on the `new_envp_pointer` passed.
 */
int modify_exec_env(char *const *envp, char ***new_envp_pointer,
    char** env_termux_proc_self_exe, bool unset_ld_vars_from_env);



/**
 * Modify the arguments for `execve()`.
 *
 * If `interpreter_set` is set, then `argv[0]` will be set to
 * `file_header_info.orig_interpreter_path`, otherwise the original
 * `argv[0]` passed to `execve()` will be preserved.
 *
 * If `system_linker_exec` is `true`, then `argv[1]` will be set
 * to `executable_path` to be executed by the linker.
 *
 * If `interpreter_set` is set, then `file_header_info.interpreter_arg`
 * will be appended if set, followed by the `orig_executable_path`
 * passed to `execve()`.
 *
 * Any additional arguments to `execve()` will be appended after this.
 *
 * @param argv The current arguments pointer.
 * @param new_argv_pointer The new arguments pointer to set.
 * @param orig_executable_path The originnal executable path passed to
 *                             `execve()`.
 * @param executable_path The **normalized** executable or interpreter
 *                        path that will actually be executed.
 * @param interpreter_set Whether a interpreter is set in the executable
 *                        file.
 * @param system_linker_exec Whether `system_linker_exec` is enabled.
 * @param info The `file_header_info` for the executable file.
 * @return Returns `0` if successfully modified the args, otherwise
 * `-1` on failures. Its the callers responsibility to call `free()`
 * on the `new_argv_pointer` passed.
 */
int modify_exec_args(char *const *argv, const char ***new_argv_pointer,
    const char *orig_executable_path, const char *executable_path,
    bool interpreter_set, bool system_linker_exec, struct file_header_info *info);

#endif // EXEC_H

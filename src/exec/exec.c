#define _GNU_SOURCE
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/api-level.h>
#endif

#include "../data/data_utils.h"
#include "../file/file_utils.h"
#include "../logger/logger.h"
#include "../os/selinux_utils.h"
#include "../termux/termux_env.h"
#include "../termux/termux_files.h"
#include "exec.h"

static const char* LOG_TAG = "exec";



/**
 * Call the `execve(2)` system call.
 *
 * - https://man7.org/linux/man-pages/man2/execve.2.html
 */
int execve_syscall(const char *executable_path, char *const argv[], char *const envp[]);

/**
 * Intercept and make changes required for termux and then call
 * `execve_syscall()` to execute the `execve(2)` system call.
 */
int execve_intercept(const char *orig_executable_path, char *const argv[], char *const envp[]);



int execve_hook(bool hook, const char *executable_path, char *const argv[], char *const envp[]) {
    bool debugLoggingEnabled = getCurrentLogLevel() >= LOG_LEVEL_NORMAL;

    if (debugLoggingEnabled) {
        if (hook) {
            logErrorDebug(LOG_TAG, "<----- execve() hooked ----->");
        }
        logErrorVerbose(LOG_TAG, "executable = '%s'", executable_path);
        int tmp_argv_count = 0;
        while (argv[tmp_argv_count] != NULL) {
            logErrorVerbose(LOG_TAG, "   argv[%d] = '%s'", tmp_argv_count, argv[tmp_argv_count]);
            tmp_argv_count++;
        }
    }

    int result;
    if (!is_termux_exec_execve_intercept_enabled()) {
        logErrorVerbose(LOG_TAG, "Intercept execve disabled");
        result = execve_syscall(executable_path, argv, envp);
    } else {
        logErrorVerbose(LOG_TAG, "Intercepting execve");
        result = execve_intercept(executable_path, argv, envp);
    }

    if (debugLoggingEnabled) {
        int saved_errno = errno;
        logErrorDebug(LOG_TAG, "<----- execve() failed ----->");
        errno = saved_errno;
    }

    return result;
}

int execve_syscall(const char *executable_path, char *const argv[], char *const envp[]) {
    return syscall(SYS_execve, executable_path, argv, envp);
}

int execve_intercept(const char *orig_executable_path, char *const argv[], char *const envp[]) {
    bool debugLoggingEnabled = getCurrentLogLevel() >= LOG_LEVEL_NORMAL;
    bool verboseLoggingEnabled = getCurrentLogLevel() >= LOG_LEVEL_VERBOSE;

    char termux_rootfs_dir_buf[TERMUX__ROOTFS_DIR_MAX_LEN];
    const char *termux_rootfs_dir = get_termux_rootfs_dir_from_env_or_default(LOG_TAG,
        termux_rootfs_dir_buf, sizeof(termux_rootfs_dir_buf));
    if (termux_rootfs_dir == NULL) {
            return -1;
    }



    // - We normalize the path to remove `.` and `..` path components,
    //   and duplicate path separators `//`, but without resolving symlinks.
    //   For instance, `$TERMUX__PREFIX/bin/ls` is a symlink to `$TERMUX__PREFIX/bin/coreutils`,
    //   but we need to execute `$TERMUX__PREFIX/bin/ls` `/system/bin/linker $TERMUX__PREFIX/bin/ls`
    //   so that coreutils knows what to execute.
    // - For an absolute path, we need to normalize first so that an
    //   unnormalized prefix like `/usr/./bin` is replaced with `/usr/bin`
    //   so that `termux_prefix_path()` can successfully match it to
    //   replace prefix with termux rootfs prefix.
    // - For a relative path, we do not replace prefix with termux rootfs
    //   prefix, but instead prefix the current working directory (`cwd`).
    //   If `cwd` is set to `/bin` and `./sh` is executed, then
    //   `/bin/sh` should be executed instead of `$TERMUX__PREFIX/bin/sh`.
    //   Moreover, to handle the case where the executable path contains
    //   double dot `..` path components like `../sh`, we need to
    //   prefix the `cwd` first and then normalize the path, otherwise
    //   `normalize_path()` will return `null`, as unknown path
    //   components cannot be removed from a path.
    //   If instead on returning `null`, `normalize_path()` just
    //   removed the extra leading double dot components from the start
    //   and then we prefixed with `cwd`, then final path will be wrong
    //   since double dot path components would have been removed before
    //   they could be used to remove path components of the `cwd`.
    // - $TERMUX_EXEC__PROC_SELF_EXE will be later set to the processed path
    //   (normalized/absolutized/prefixed) that will actually be executed.
    char executable_path_buffer[strlen(orig_executable_path) + 1];
    strcpy(executable_path_buffer, orig_executable_path);
    const char *executable_path = executable_path_buffer;


    char processed_executable_path_buffer[PATH_MAX];
    if (executable_path[0] == '/') {
        // If path is absolute, then normalize first and then replace rootfs
        executable_path = normalize_path(executable_path_buffer, false, true);
        if (executable_path == NULL) {
            logStrerrorDebug(LOG_TAG, "Failed to normalize executable path '%s'", orig_executable_path);
            return -1;
        }

        if (verboseLoggingEnabled && strcmp(orig_executable_path, executable_path) != 0) {
            logErrorVVerbose(LOG_TAG, "normalized_executable: '%s'", executable_path);
        }
        const char *normalized_executable_path = executable_path;


        executable_path = termux_prefix_path(LOG_TAG, termux_rootfs_dir, executable_path,
            processed_executable_path_buffer, sizeof(processed_executable_path_buffer));
        if (executable_path == NULL) {
            logStrerrorDebug(LOG_TAG, "Failed to prefix normalized executable path '%s'", normalized_executable_path);
            return -1;
        }

        if (verboseLoggingEnabled && strcmp(normalized_executable_path, executable_path) != 0) {
            logErrorVVerbose(LOG_TAG, "prefixed_executable: '%s'", executable_path);
        }
    } else {
        // If path is relative, then absolutize first and then normalize
        executable_path = absolutize_path(executable_path,
            processed_executable_path_buffer, sizeof(processed_executable_path_buffer));
        if (executable_path == NULL) {
            logStrerrorDebug(LOG_TAG, "Failed to convert executable path '%s' to an absolute path", orig_executable_path);
            return -1;
        }

        if (verboseLoggingEnabled && strcmp(orig_executable_path, executable_path) != 0) {
            logErrorVVerbose(LOG_TAG, "absolutized_executable: '%s'", executable_path);
        }


        char absolute_executable_path_buffer[strlen(processed_executable_path_buffer) + 1];
        strcpy(absolute_executable_path_buffer, processed_executable_path_buffer);

        executable_path = normalize_path(processed_executable_path_buffer, false, true);
        if (executable_path == NULL) {
            logStrerrorDebug(LOG_TAG, "Failed to normalize absolutized executable path '%s'", absolute_executable_path_buffer);
            return -1;
        }

        if (verboseLoggingEnabled && strcmp(absolute_executable_path_buffer, executable_path) != 0) {
            logErrorVVerbose(LOG_TAG, "normalized_executable: '%s'", executable_path);
        }
    }

    const char *processed_executable_path = executable_path;

    // - https://man7.org/linux/man-pages/man2/access.2.html
    if (access(executable_path, X_OK) != 0) {
        // Error out if the file does not exist or is not executable
        // fd paths like to a pipes should not be executable either and
        // they cannot be seek-ed back if interpreter were to be read.
        // https://github.com/bminor/bash/blob/bash-5.2/shell.c#L1649
        logStrerrorDebug(LOG_TAG, "Failed to access executable path '%s'", processed_executable_path);
        return -1;
    }

    // - https://man7.org/linux/man-pages/man2/open.2.html
    int fd = open(executable_path, O_RDONLY);
    if (fd == -1) {
        errno = ENOENT;
        logStrerrorDebug(LOG_TAG, "Failed to open executable path '%s'", processed_executable_path);
        return -1;
    }



    char header[FILE_HEADER__BUFFER_LEN];
    ssize_t read_bytes = read(fd, header, sizeof(header) - 1);
    // Ensure read was successful, path could be a directory and EISDIR will be returned.
    // - https://man7.org/linux/man-pages/man2/read.2.html
    if (read_bytes < 0) {
        logStrerrorDebug(LOG_TAG, "Failed to read executable path '%s'", processed_executable_path);
        return -1;
    }
    close(fd);

    struct file_header_info info = {
        .interpreter_path = NULL,
        .interpreter_arg = NULL,
    };

    if (inspect_file_header(termux_rootfs_dir, header, read_bytes, &info) != 0) {
        return -1;
    }

    bool interpreter_set = info.interpreter_path != NULL;
    if (!info.is_elf && !interpreter_set) {
        errno = ENOEXEC;
        logStrerrorDebug(LOG_TAG, "Not an ELF or no shebang in executable path '%s'", processed_executable_path);
        return -1;
    }

    if (interpreter_set) {
        executable_path = info.interpreter_path;
    }




    // Check if system_linker_exec is required.
    int system_linker_exec = should_system_linker_exec(executable_path, termux_rootfs_dir);
    if (system_linker_exec < 0) {
        return -1;
    }



    bool modify_env = false;
    bool unset_ld_vars_from_env = should_unset_ld_vars_from_env(info.is_non_native_elf, executable_path);
    logErrorVVerbose(LOG_TAG, "unset_ld_vars_from_env: '%d'", unset_ld_vars_from_env);

    if (unset_ld_vars_from_env && are_vars_in_env(envp, LD_VARS_TO_UNSET, LD_VARS_TO_UNSET_SIZE)) {
        modify_env = true;
    }



    // If `system_linker_exec` is going to be used, then set `TERMUX_EXEC__PROC_SELF_EXE`
    // environment variable to `processed_executable_path`, otherwise
    // unset it if it is already set.
    char *env_termux_proc_self_exe = NULL;
    if (system_linker_exec) {
        modify_env = true;
        logErrorVVerbose(LOG_TAG, "set_proc_self_exe_var_in_env: '%d'", true);

        if (asprintf(&env_termux_proc_self_exe, "%s%s", ENV_PREFIX__TERMUX_EXEC__PROC_SELF_EXE, processed_executable_path) == -1) {
            errno = ENOMEM;
            logStrerrorDebug(LOG_TAG, "asprintf failed for '%s%s'", ENV_PREFIX__TERMUX_EXEC__PROC_SELF_EXE, processed_executable_path);
            return -1;
        }
    } else {
        const char *proc_self_exe_var[] = { ENV_PREFIX__TERMUX_EXEC__PROC_SELF_EXE };
        if (are_vars_in_env(envp, proc_self_exe_var, 1)) {
            logErrorVVerbose(LOG_TAG, "unset_proc_self_exe_var_from_env: '%d'", true);
            modify_env = true;
        }
    }

    logErrorVVerbose(LOG_TAG, "modify_env: '%d'", modify_env);



    char **new_envp = NULL;
    if (modify_env) {
        if (modify_exec_env(envp, &new_envp, &env_termux_proc_self_exe, unset_ld_vars_from_env) != 0 ||
            new_envp == NULL) {
            logErrorDebug(LOG_TAG, "Failed to create modified exec env");
            free(env_termux_proc_self_exe);
            return -1;
        }

        envp = new_envp;
    }



    const bool modify_args = system_linker_exec || interpreter_set;
    logErrorVVerbose(LOG_TAG, "modify_args: '%d'", modify_args);

    const char **new_argv = NULL;
    if (modify_args) {
        if (modify_exec_args(argv, &new_argv, orig_executable_path, executable_path,
            interpreter_set, system_linker_exec, &info) != 0 ||
            new_argv == NULL) {
            logErrorDebug(LOG_TAG, "Failed to create modified exec args");
            free(env_termux_proc_self_exe);
            free(new_envp);
            return -1;
        }

        // Replace executable path if wrapping with linker
        if (system_linker_exec) {
            executable_path = SYSTEM_LINKER_PATH;
        }

        argv = (char **) new_argv;
    }



    if (debugLoggingEnabled) {
        logErrorVerbose(LOG_TAG, "Calling syscall execve");
        logErrorVerbose(LOG_TAG, "executable = '%s'", executable_path);
        int tmp_argv_count = 0;
        int arg_count = 0;
        while (argv[tmp_argv_count] != NULL) {
            logErrorVerbose(LOG_TAG, "   argv[%d] = '%s'", arg_count++, argv[tmp_argv_count]);
            tmp_argv_count++;
        }
    }

    int syscall_ret = execve_syscall(executable_path, argv, envp);
    int saved_errno = errno;
    logStrerrorDebug(LOG_TAG, "execve() syscall failed for executable path '%s'", executable_path);
    free(env_termux_proc_self_exe);
    free(new_envp);
    free(new_argv);
    errno = saved_errno;
    return syscall_ret;
}



int inspect_file_header(char const *termux_rootfs_dir, char *header, size_t header_len,
    struct file_header_info *info) {
    if (header_len >= 20 && !memcmp(header, ELFMAG, SELFMAG)) {
        info->is_elf = true;
        Elf32_Ehdr *ehdr = (Elf32_Ehdr *)header;
        if (ehdr->e_machine != EM_NATIVE) {
            info->is_non_native_elf = true;
        }
        return 0;
    }

    if (header_len < 3 || !(header[0] == '#' && header[1] == '!')) {
        return 0;
    }

    // Check if the header contains a newline to end the shebang line:
    char *newline_location = memchr(header, '\n', header_len);
    if (newline_location == NULL) {
        return 0;
    }

    // Strip whitespace at end of shebang:
    while (*(newline_location - 1) == ' ') {
        newline_location--;
    }

    // Null terminate the shebang line:
    *newline_location = 0;

    // Skip whitespace to find interpreter start:
    char const *interpreter = header + 2;
    while (*interpreter == ' ') {
        interpreter++;
    }
    if (interpreter == newline_location) {
        // Just a blank line up until the newline.
        return 0;
    }

    // Check for whitespace following the interpreter:
    char *whitespace_pos = strchr(interpreter, ' ');
    if (whitespace_pos != NULL) {
        // Null-terminate the interpreter string.
        *whitespace_pos = 0;

        // Find start of argument:
        char *interpreter_arg = whitespace_pos + 1;
        while (*interpreter_arg != 0 && *interpreter_arg == ' ') {
            interpreter_arg++;
        }
        if (interpreter_arg != newline_location) {
            strncpy(info->interpreter_arg_buffer, interpreter_arg, sizeof(info->interpreter_arg_buffer) - 1);
            info->interpreter_arg = info->interpreter_arg_buffer;
        }
    }

    #ifndef TERMUX_EXEC__RUNNING_TESTS
    logErrorVVerbose(LOG_TAG, "interpreter_path: '%s'", interpreter);
    #endif

    // argv[0] must be set to the original interpreter set in the file even
    // if it is a relative path and its absolute path is to be executed.
    info->orig_interpreter_path = interpreter;



    bool verboseLoggingEnabled = getCurrentLogLevel() >= LOG_LEVEL_VERBOSE;
    (void)verboseLoggingEnabled;
    size_t interpreter_path_buffer_len = sizeof(info->interpreter_path_buffer);

    char interpreter_path_buffer[strlen(interpreter) + 1];
    strcpy(interpreter_path_buffer, interpreter);
    char *interpreter_path = interpreter_path_buffer;


    if (interpreter_path[0] == '/') {
        // If path is absolute, then normalize first and then replace rootfs
        interpreter_path = normalize_path(interpreter_path, false, true);
        if (interpreter_path == NULL) {
            logStrerrorDebug(LOG_TAG, "Failed to normalize interpreter path '%s'", info->orig_interpreter_path);
            return -1;
        }
        #ifndef TERMUX_EXEC__RUNNING_TESTS
        if (verboseLoggingEnabled && strcmp(info->orig_interpreter_path, interpreter_path) != 0) {
            logErrorVVerbose(LOG_TAG, "normalized_interpreter: '%s'", interpreter_path);
        }
        #endif


        const char *normalized_interpreter_path = interpreter_path;

        info->interpreter_path = termux_prefix_path(LOG_TAG, termux_rootfs_dir,
            interpreter_path, info->interpreter_path_buffer, interpreter_path_buffer_len);
        if (info->interpreter_path == NULL) {
            logStrerrorDebug(LOG_TAG, "Failed to prefix normalized interpreter path '%s'", normalized_interpreter_path);
            return -1;
        }

        #ifndef TERMUX_EXEC__RUNNING_TESTS
        if (verboseLoggingEnabled && strcmp(normalized_interpreter_path, info->interpreter_path) != 0) {
            logErrorVVerbose(LOG_TAG, "prefixed_interpreter: '%s'", info->interpreter_path);
        }
        #endif
    } else {
        char processed_interpreter_path_buffer[PATH_MAX];

        // If path is relative, then absolutize first and then normalize
        interpreter_path = absolutize_path(interpreter_path,
            processed_interpreter_path_buffer, sizeof(processed_interpreter_path_buffer));
        if (interpreter_path == NULL) {
            logStrerrorDebug(LOG_TAG, "Failed to convert interpreter path '%s' to an absolute path", info->orig_interpreter_path);
            return -1;
        }

        #ifndef TERMUX_EXEC__RUNNING_TESTS
        if (verboseLoggingEnabled && strcmp(info->orig_interpreter_path, interpreter_path) != 0) {
            logErrorVVerbose(LOG_TAG, "absolute_interpreter: '%s'", interpreter_path);
        }
        #endif


        char absolute_interpreter_path_buffer[strlen(processed_interpreter_path_buffer) + 1];
        strcpy(absolute_interpreter_path_buffer, processed_interpreter_path_buffer);

        interpreter_path = normalize_path(processed_interpreter_path_buffer, false, true);
        if (interpreter_path == NULL) {
            logStrerrorDebug(LOG_TAG, "Failed to normalize absolutized interpreter path '%s'", absolute_interpreter_path_buffer);
            return -1;
        }

        #ifndef TERMUX_EXEC__RUNNING_TESTS
        if (verboseLoggingEnabled && strcmp(absolute_interpreter_path_buffer, interpreter_path) != 0) {
            logErrorVVerbose(LOG_TAG, "normalized_interpreter: '%s'", interpreter_path);
        }
        #endif


        size_t processed_interpreter_path_len = strlen(interpreter_path);
        if (interpreter_path_buffer_len <= processed_interpreter_path_len) {
            #ifndef TERMUX_EXEC__RUNNING_TESTS
            logErrorDebug(LOG_TAG, "The processed interpreter path '%s' with length '%zu' is too long to fit in the buffer with length '%zu'",
                interpreter_path, processed_interpreter_path_len, interpreter_path_buffer_len);
            #endif
            errno = ENAMETOOLONG;
            return -1;
        }

        strncpy(info->interpreter_path_buffer, interpreter_path, interpreter_path_buffer_len);
        info->interpreter_path = info->interpreter_path_buffer;
    }


    #ifndef TERMUX_EXEC__RUNNING_TESTS
    if (verboseLoggingEnabled && info->interpreter_arg != NULL) {
        logErrorVVerbose(LOG_TAG, "interpreter_arg: '%s'", info->interpreter_arg);
    }
    #endif

    return 0;
}



int should_system_linker_exec(const char *executable_path, const char *termux_rootfs_dir) {
    int system_linker_exec_config = get_termux_exec_system_linker_exec_config();
    logErrorVVerbose(LOG_TAG, "system_linker_exec_config: '%d'", system_linker_exec_config);

    bool system_linker_exec = false;
    if (system_linker_exec_config == 0) { // disable
        system_linker_exec = false;

    } else if (system_linker_exec_config == 2) { // force
            bool system_linker_exec_available = false;
            #ifdef __ANDROID__
                system_linker_exec_available = android_get_device_api_level() >= 29;
                logErrorVVerbose(LOG_TAG, "system_linker_exec_available: '%d'", system_linker_exec_available);
            #endif

            int is_executable_under_termux_rootfs_dir = is_path_under_termux_rootfs_dir(LOG_TAG,
                executable_path, termux_rootfs_dir);
            if (is_executable_under_termux_rootfs_dir < 0) {
                return -1;
            }
            logErrorVVerbose(LOG_TAG, "is_executable_under_termux_rootfs_dir: '%d'",
                is_executable_under_termux_rootfs_dir == 0 ? true : false);

            system_linker_exec = system_linker_exec_available && is_executable_under_termux_rootfs_dir == 0;

    } else { // enable
        if (system_linker_exec_config != 1) {
            logErrorDebug(LOG_TAG, "Warning: Ignoring invalid system_linker_exec_config value and using '1' instead");
        }

        bool app_data_file_exec_exempted = false;
        (void)app_data_file_exec_exempted;

        #ifdef __ANDROID__
        // The `android_get_device_api_level()` method was added in api 29 itself,
        // but bionic provides an inline variant for backward compatibility.
        // The method does not cache api level, so if it is needed in multiple places,
        // then do it.
        // - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/include/android/api-level.h;l=191-209
        // - https://cs.android.com/android/_/android/platform/bionic/+/c0f46564528c7bec8d490e62633e962f2007b8f4
        if (android_get_device_api_level() >= 29) {
            // If running as root or shell user, then the process will be assigned a different
            // process context like `PROCESS_CONTEXT__AOSP_SU`, `PROCESS_CONTEXT__MAGISK_SU`
            // or `PROCESS_CONTEXT__SHELL`, which will not be the same
            // as the one that's exported in `ENV__TERMUX__SE_PROCESS_CONTEXT`,
            // so we need to check effective uid equals `0` or `2000` instead.
            // Moreover, other su providers may have different contexts,
            // so we cannot just check AOSP or MAGISK contexts.
            // - https://man7.org/linux/man-pages/man2/getuid.2.html
            uid_t uid = geteuid();
            if (uid == 0 || uid == 2000) {
                logErrorVVerbose(LOG_TAG, "uid: '%d'", uid);
                app_data_file_exec_exempted = true;
            } else {
                char se_process_context[80];
                bool get_se_process_context_success = false;

                if (get_se_process_context_from_env(LOG_TAG, ENV__TERMUX__SE_PROCESS_CONTEXT,
                    se_process_context, sizeof(se_process_context))) {
                    logErrorVVerbose(LOG_TAG, "se_process_context_from_env: '%s'", se_process_context);
                    get_se_process_context_success = true;
                } else if (get_se_process_context_from_file(LOG_TAG,
                    se_process_context, sizeof(se_process_context))) {
                    logErrorVVerbose(LOG_TAG, "se_process_context_from_file: '%s'", se_process_context);
                    get_se_process_context_success = true;
                }

                if (get_se_process_context_success) {
                    app_data_file_exec_exempted = string_starts_with(se_process_context, PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_25) ||
                        string_starts_with(se_process_context, PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_27);
                }
            }

            logErrorVVerbose(LOG_TAG, "app_data_file_exec_exempted: '%d'", app_data_file_exec_exempted);

            if (!app_data_file_exec_exempted) {
                int is_executable_under_termux_rootfs_dir = is_path_under_termux_rootfs_dir(LOG_TAG,
                    executable_path, termux_rootfs_dir);
                if (is_executable_under_termux_rootfs_dir < 0) {
                    return -1;
                }

                system_linker_exec = is_executable_under_termux_rootfs_dir == 0;
                logErrorVVerbose(LOG_TAG, "is_executable_under_termux_rootfs_dir: '%d'", system_linker_exec);
            }
        }
        #endif
    }

    logErrorVVerbose(LOG_TAG, "system_linker_exec: '%d'", system_linker_exec);

    return 1 ? system_linker_exec : 0;
}



bool should_unset_ld_vars_from_env(bool is_non_native_elf, const char *executable_path) {
    return is_non_native_elf ||
        (string_starts_with(executable_path, "/system/") &&
        strcmp(executable_path, "/system/bin/sh") != 0 &&
        strcmp(executable_path, "/system/bin/linker") != 0 &&
        strcmp(executable_path, "/system/bin/linker64") != 0);
}

int modify_exec_env(char *const *envp, char ***new_envp_pointer,
    char** env_termux_proc_self_exe, bool unset_ld_vars_from_env) {
    int env_count = 0;
    while (envp[env_count] != NULL) {
        env_count++;
    }

    // Allocate new environment variable array. Size + 2 since
    // we might perhaps append a TERMUX_EXEC__PROC_SELF_EXE variable and
    // we will also NULL terminate.
    size_t new_envp_size = (sizeof(char *) * (env_count + 2));
    void* result = malloc(new_envp_size);
    if (result == NULL) {
        logStrerrorDebug(LOG_TAG, "The malloc called failed for new envp with size '%zu'", new_envp_size);
        return -1;
    }

    char **new_envp = (char **) result;
    *new_envp_pointer = new_envp;

    bool already_found_proc_self_exe = false;
    int index = 0;
    for (int i = 0; i < env_count; i++) {
        if (string_starts_with(envp[i], ENV_PREFIX__TERMUX_EXEC__PROC_SELF_EXE)) {
            if (env_termux_proc_self_exe != NULL && *env_termux_proc_self_exe != NULL) {
                new_envp[index++] = *env_termux_proc_self_exe;
                already_found_proc_self_exe = true;
                #ifndef TERMUX_EXEC__RUNNING_TESTS
                logErrorVVerbose(LOG_TAG, "Overwrite '%s'", *env_termux_proc_self_exe);
                #endif
            }
            #ifndef TERMUX_EXEC__RUNNING_TESTS
            else {
                logErrorVVerbose(LOG_TAG, "Unset '%s'", envp[i]);
            }
            #endif
        } else {
            bool keep = true;

            if (unset_ld_vars_from_env) {
                for (int j = 0; j < LD_VARS_TO_UNSET_SIZE; j++) {
                    if (string_starts_with(envp[i], LD_VARS_TO_UNSET[j])) {
                        keep = false;
                    }
                }
            }

            if (keep) {
                new_envp[index++] = envp[i];
            }
            #ifndef TERMUX_EXEC__RUNNING_TESTS
            else {
                logErrorVVerbose(LOG_TAG, "Unset '%s'", envp[i]);
            }
            #endif
        }
    }

    if (env_termux_proc_self_exe != NULL && *env_termux_proc_self_exe != NULL && !already_found_proc_self_exe) {
        new_envp[index++] = *env_termux_proc_self_exe;
        #ifndef TERMUX_EXEC__RUNNING_TESTS
        logErrorVVerbose(LOG_TAG, "Set '%s'", *env_termux_proc_self_exe);
        #endif
    }

    // Null terminate.
    new_envp[index] = NULL;

    return 0;
}



int modify_exec_args(char *const *argv, const char ***new_argv_pointer,
    const char *orig_executable_path, const char *executable_path,
    bool interpreter_set, bool system_linker_exec, struct file_header_info *info) {
    int args_count = 0;
    while (argv[args_count] != NULL) {
        args_count++;
    }

    size_t new_argv_size = (sizeof(char *) * (args_count + 2));
    void* result = malloc(new_argv_size);
    if (result == NULL) {
        logStrerrorDebug(LOG_TAG, "The malloc called failed for new argv with size '%zu'", new_argv_size);
        return -1;
    }

    const char **new_argv = (const char **) result;
    *new_argv_pointer = new_argv;

    int index = 0;

    if (interpreter_set) {
        // Use original interpreter path set in executable file as is
        new_argv[index++] = info->orig_interpreter_path;
    } else {
        // Preserver original `argv[0]` to `execve()`
        new_argv[index++] = argv[0];
    }

    // Add executable path if wrapping with linker
    if (system_linker_exec) {
        new_argv[index++] = executable_path;
    }

    // Add interpreter argument and script path if executing a script with shebang
    if (interpreter_set) {
        if (info->interpreter_arg != NULL) {
            new_argv[index++] = info->interpreter_arg;
        }
        new_argv[index++] = orig_executable_path;
    }

    for (int i = 1; i < args_count; i++) {
        new_argv[index++] = argv[i];
    }

    // Null terminate.
    new_argv[index] = NULL;

    return 0;
}

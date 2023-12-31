#define _GNU_SOURCE
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/api-level.h>
#endif

#include "../src/data/assert_utils.h"
#include "../src/data/data_utils.h"
#include "../src/logger/logger.h"
#include "../src/exec/exec_variants.h"
#include "../src/termux/termux_env.h"

static const char* LOG_TAG = "runtime_tests";

static uid_t UID;

#define TERMUX_EXEC__TESTS_PATH TERMUX__PREFIX "/libexec/installed-tests/termux-exec"
#define TERMUX_EXEC__TEST_FILES_PATH TERMUX_EXEC__TESTS_PATH "/files"
#define TERMUX_EXEC__EXEC_TEST_FILES_PATH TERMUX_EXEC__TEST_FILES_PATH "/exec"

extern char **environ;

static void init();
static void initLogger();



typedef struct {
    bool is_child;
    int cpid;
    int exit_code;
    int status;
    int pipe_fds[2];
    FILE *pipe_file;
    char *output;
    bool return_output;
    size_t output_initial_buffer;
} fork_info;

#define INIT_FORK_INFO(X) fork_info X = {.cpid = -1, .exit_code = -1, .pipe_file = NULL, .output = NULL, .return_output = true, .output_initial_buffer = 255}

void cleanup_fork(fork_info *info) {
    if (info->pipe_fds[0] != -1)
        close(info->pipe_fds[0]);
    if (info->pipe_fds[1] != -1)
        close(info->pipe_fds[1]);
    if (info->pipe_file != NULL)
        fclose(info->pipe_file);
    if (info->output != NULL)
        free(info->output);
}

void exit_fork_with_error(fork_info *info, int exit_code) {
    cleanup_fork(info);
    if (!info->is_child && info->cpid != -1 && info->exit_code == -1) {
        kill(info->cpid, SIGKILL);
    }
    exit(exit_code);
}

int fork_child(fork_info *info) {
    pid_t cpid, w;


    // Create pipe for capturing child stdout and stderr
    if (pipe(info->pipe_fds) == -1) {
        logStrerror(LOG_TAG, "pipe() failed");
        return 1;
    }

    cpid = fork();
    if (cpid == -1) {
        logStrerror(LOG_TAG, "fork() failed");
        exit(1);
    }

    // If in child
    if (cpid == 0) {
        info->is_child = true;

        initLogger();

        // Redirect stdout/stderr to pipe -> send output to parent
        if (dup2(info->pipe_fds[1], STDOUT_FILENO) == -1) {
            logStrerror(LOG_TAG, "Child: Failed to redirect stdout");
            exit_fork_with_error(info, 1);
        }

        if (dup2(info->pipe_fds[1], STDERR_FILENO) == -1) {
            logStrerror(LOG_TAG, "Child: Failed to redirect stderr");
            exit_fork_with_error(info, 1);
        }

        // Close both pipe ends (pipe_fds[0] belongs to parent, pipe_fds[1] no longer needed after dup2)
        close(info->pipe_fds[0]);
        close(info->pipe_fds[1]);
        info->pipe_fds[0] = info->pipe_fds[1] = -1;

        // Set buffering for stdout/stderr
        if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
            logStrerror(LOG_TAG, "Child: Failed to set buffering for stdout");
            exit_fork_with_error(info, 1);
        }
        if (setvbuf(stderr, NULL, _IONBF, 0) != 0) {
            logStrerror(LOG_TAG, "Child: Failed to set buffering for stderr");
            exit_fork_with_error(info, 1);
        }

        // Let caller child logic continue
        return 0;
    }
    // If in parent
    else {
        info->is_child = false;

        char *buffer = NULL;
        long unsigned int buf_size;
        long unsigned int buf_data;
        long unsigned int buf_free;

        // Close child's side of pipe
        close(info->pipe_fds[1]);

        // Open parent's side of pipe for reading
        info->pipe_file = fdopen(info->pipe_fds[0], "r");
        if (info->pipe_file == NULL) {
            logStrerror(LOG_TAG, "Parent: Failed to open pipe for read child output");
            exit_fork_with_error(info, 1);
        }

        // Allocate buffer to store child's output
        buf_size = info->output_initial_buffer;
        buf_data = 0;
        buf_free = buf_size;
        buffer = (char *) malloc(buf_size);
        if (buffer == NULL) {
            logStrerror(LOG_TAG, "Parent: Failed to allocate initial buffer to store child output");
            exit_fork_with_error(info, 1);
        }

        // Read child output until EOF
        while (!feof(info->pipe_file)) {
            int n = fread(buffer + buf_data, 1, buf_free, info->pipe_file);
            if (n <= 0 && ferror(info->pipe_file) != 0) {
                logStrerror(LOG_TAG, "Parent: Failed to read from pipe of child output");
                exit_fork_with_error(info, 1);
            }

            buf_data += n;
            buf_free -= n;
            if (buf_free <= 0) {
                buf_free += buf_size;
                buf_size *= 2;
                char *newbuf = (char *) realloc(buffer, buf_size);
                if (newbuf == NULL) {
                    logStrerror(LOG_TAG, "Parent: Failed to reallocate buffer to store child output");
                    exit_fork_with_error(info, 1);
                }
                buffer = newbuf;
            }
        }
        if (buf_data > 0 && buffer[buf_data-1] == '\n') // remove trailing newline
            buf_data--;
        buffer[buf_data++] = '\0';

        // Resize buffer to final size
        buf_size = buf_data;
        buf_free = 0;
        char *newbuf = (char *) realloc(buffer, buf_size);
        if (newbuf == NULL) {
            logStrerror(LOG_TAG, "Parent: Failed to reallocate buffer to store final child output");
            exit_fork_with_error(info, 1);
        }
        buffer = newbuf;

        // Wait for child to finish
        // If in parent, wait for child to exit
        w = waitpid(cpid, &info->status, WUNTRACED | WCONTINUED);
        if (w == -1) {
            logStrerror(LOG_TAG, "Parent: waitpid() failed");
            exit(1);
        }

        // Close pipe file and pipe
        fclose(info->pipe_file);
        close(info->pipe_fds[0]);

        // Return results using provided references
        if (info->return_output) {
            info->output = buffer;
        } else {
            free(buffer);
        }

        // Let caller parent logic continue
        info->exit_code = WEXITSTATUS(info->status);
        return 0;
    }
}




#if defined __ANDROID__ && __ANDROID_API__ >= 28
#define FEXECVE_SUPPORTED 1
#endif

#ifndef __ANDROID__
#define FEXECVE_SUPPORTED 1
#endif

#if defined FEXECVE_SUPPORTED
#define FEXECVE_CALL_IMPL()                                                                        \
int fexecve_call(int fd, char *const *argv, char *const *envp) {                                   \
    return fexecve(fd, argv, envp);                                                                \
}
#else
#define FEXECVE_SUPPORTED 0
#define FEXECVE_CALL_IMPL()                                                                        \
int fexecve_call(int fd, char *const *argv, char *const *envp) {                                   \
    (void)fd; (void)argv; (void)envp;                                                              \
    logStrerror(LOG_TAG, "fexecve not supported on __ANDROID_API__ %d and requires api level >= %d", __ANDROID_API__, 28); \
    return -1;                                                                                     \
}
#endif

FEXECVE_CALL_IMPL()
#undef FEXECVE_CALL_IMPL



#define exec_wrapper(variant, name, envp, ...)                                                     \
    if (1) {                                                                                       \
    /* Construct argv */                                                                           \
    char *argv[] = {__VA_ARGS__};                                                                  \
                                                                                                   \
    switch (variant) {                                                                             \
        case ExecVE: {                                                                             \
            actual_return_value = execve(name, argv, envp);                                        \
            break;                                                                                 \
        } case ExecL: {                                                                            \
            actual_return_value = execl(name, __VA_ARGS__);                                        \
            break;                                                                                 \
        } case ExecLP: {                                                                           \
            actual_return_value = execlp(name, __VA_ARGS__);                                       \
            break;                                                                                 \
        } case ExecLE: {                                                                           \
            actual_return_value = execle(name, __VA_ARGS__, envp);                                 \
            break;                                                                                 \
        } case ExecV: {                                                                            \
            actual_return_value = execv(name, argv);                                               \
            break;                                                                                 \
        } case ExecVP: {                                                                           \
            actual_return_value = execvp(name, argv);                                              \
            break;                                                                                 \
        } case ExecVPE: {                                                                          \
            actual_return_value = execvpe(name, argv, envp);                                       \
            break;                                                                                 \
        } case FExecVE: {                                                                          \
            int fd = open(name, 0);                                                                \
            if (fd == -1) {                                                                        \
                logStrerror(LOG_TAG, "open() call failed");                                        \
                exit(1);                                                                           \
            }                                                                                      \
                                                                                                   \
            actual_return_value = fexecve_call(fd, argv, envp);                                    \
            close(fd);                                                                             \
            break;                                                                                 \
        } default: {                                                                               \
            logStrerror(LOG_TAG, "Unknown exec() variant %d", variant);                            \
            exit(1);                                                                               \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    } else ((void)0)

#define run_exec_test(test_name,                                                                   \
    expected_return_value, expected_errno,                                                         \
    expected_exit_code, expected_output_regex, expected_output_regex_flags,                        \
    variant, name, envp, ...)                                                                      \
    if (1) {                                                                                       \
    logVVerbose(LOG_TAG, "Running \"%s\" \"%s()\" test", test_name, EXEC_VARIANTS_STR[variant]);    \
                                                                                                   \
    INIT_FORK_INFO(info);                                                                          \
    int result = fork_child(&info);                                                                \
    if (result != 0) {                                                                             \
        logError(LOG_TAG, "Unexpected return value for fork_child '%d'", result);                  \
        exit(1);                                                                                   \
    }                                                                                              \
                                                                                                   \
    if (info.is_child) {                                                                           \
        int actual_return_value;                                                                   \
        exec_wrapper(variant, name, envp, __VA_ARGS__);                                            \
        int actual_errno = errno;                                                                  \
        int test_failed = 0;                                                                       \
        if (actual_return_value != expected_return_value) {                                        \
            logError(LOG_TAG, "FAILED: \"%s\" \"%s()\" test", test_name, EXEC_VARIANTS_STR[variant]); \
            logError(LOG_TAG, "Expected return_value does not equal actual return_value");         \
            test_failed=1;                                                                         \
        } else if (actual_errno != expected_errno) {                                               \
            logError(LOG_TAG, "FAILED: \"%s\" \"%s()\" test", test_name, EXEC_VARIANTS_STR[variant]); \
            logError(LOG_TAG, "Expected errno does not equal actual errno");                       \
            test_failed=1;                                                                         \
        }                                                                                          \
                                                                                                   \
        if (test_failed == 1) {                                                                    \
            logError(LOG_TAG, "actual_return_value: \"%d\"", actual_return_value);                 \
            logError(LOG_TAG, "expected_return_value: \"%d\"", expected_return_value);             \
            logError(LOG_TAG, "actual_errno: \"%d\"", actual_errno);                               \
            logError(LOG_TAG, "expected_errno: \"%d\"", expected_errno);                           \
            exit_fork_with_error(&info, 100);                                                      \
        } else {                                                                                   \
            exit(0);                                                                               \
        }                                                                                          \
    } else {                                                                                       \
        if (WIFEXITED(info.status)) {                                                              \
            ;                                                                                      \
        } else if (WIFSIGNALED(info.status)) {                                                     \
            logInfo(LOG_TAG, "Killed by signal %d\n", WTERMSIG(info.status));                      \
        } else if (WIFSTOPPED(info.status)) {                                                      \
            logInfo(LOG_TAG, "Stopped by signal %d\n", WSTOPSIG(info.status));                     \
        } else if (WIFCONTINUED(info.status)) {                                                    \
            logInfo(LOG_TAG, "Continued");                                                         \
        } else {                                                                                   \
            logInfo(LOG_TAG, "CANCELLED");                                                         \
            exit(2);                                                                               \
        }                                                                                          \
                                                                                                   \
        int actual_exit_code = info.exit_code;                                                     \
        int test_failed = 0;                                                                       \
        int regex_match_result = 1;                                                                \
        if (expected_output_regex != NULL &&                                                       \
            (regex_match_result = regex_match(info.output, expected_output_regex, expected_output_regex_flags)) != 0) { \
            logError(LOG_TAG, "FAILED: \"%s\" \"%s()\" test", test_name, EXEC_VARIANTS_STR[variant]); \
            logError(LOG_TAG, "Expected output_regex does not equal match actual output");         \
            test_failed=1;                                                                         \
        } else if (actual_exit_code != expected_exit_code) {                                       \
            logError(LOG_TAG, "FAILED: \"%s\" \"%s()\" test", test_name, EXEC_VARIANTS_STR[variant]); \
            logError(LOG_TAG, "Expected exit_code does not equal actual exit_code");               \
            test_failed=1;                                                                         \
        }                                                                                          \
                                                                                                   \
        if (test_failed == 1) {                                                                    \
            logError(LOG_TAG, "actual_exit_code: \"%d\"", actual_exit_code);                       \
            logError(LOG_TAG, "expected_exit_code: \"%d\"", expected_exit_code);                   \
            logError(LOG_TAG, "actual_output: \"%s\"", info.output);                               \
            logError(LOG_TAG, "expected_output_regex: \"%s\" (%d)", expected_output_regex, expected_output_regex_flags); \
            if (regex_match_result != 1) {                                                         \
                logError(LOG_TAG, "regex_match_result: \"%d\"", regex_match_result);               \
            }                                                                                      \
            exit_fork_with_error(&info, 100);                                                      \
        } else {                                                                                   \
            /* logDebug(LOG_TAG, "PASSED"); */                                                     \
            free(info.output);                                                                     \
            errno = 0;                                                                             \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    } else ((void)0)

#define run_all_exec_wrappers_test(test_name,                                                      \
    expected_return_value, expected_errno,                                                         \
    expected_exit_code, expected_output_regex, expected_output_regex_flags,                        \
    file, path, envp, ...)                                                                         \
    if (1) {                                                                                       \
                                                                                                   \
    logVVerbose(LOG_TAG, "Running \"%s\" \"exec()\" wrapper tests", test_name);                    \
                                                                                                   \
    {                                                                                              \
        /* ExecVE */                                                                               \
        run_exec_test(test_name, expected_return_value, expected_errno,                            \
            expected_exit_code, expected_output_regex, expected_output_regex_flags,                \
            ExecVE, path, envp, path, __VA_ARGS__);                                                \
    }                                                                                              \
                                                                                                   \
                                                                                                   \
    {                                                                                              \
        /* ExecL */                                                                                \
        run_exec_test(test_name, expected_return_value, expected_errno,                            \
            expected_exit_code, expected_output_regex, expected_output_regex_flags,                \
            ExecL, path, NULL, path, __VA_ARGS__);                                                 \
    }                                                                                              \
    {                                                                                              \
        /* ExecLP */                                                                               \
        run_exec_test(test_name, expected_return_value, expected_errno,                            \
            expected_exit_code, expected_output_regex, expected_output_regex_flags,                \
            ExecLP, file, NULL, file, __VA_ARGS__);                                                \
    }                                                                                              \
    {                                                                                              \
        /* ExecLE */                                                                               \
        run_exec_test(test_name, expected_return_value, expected_errno,                            \
            expected_exit_code, expected_output_regex, expected_output_regex_flags,                \
            ExecLE, path, envp, path, __VA_ARGS__);                                                \
    }                                                                                              \
                                                                                                   \
                                                                                                   \
    {                                                                                              \
        /* ExecV */                                                                                \
        run_exec_test(test_name, expected_return_value, expected_errno,                            \
            expected_exit_code, expected_output_regex, expected_output_regex_flags,                \
            ExecV, path, NULL, path, __VA_ARGS__);                                                 \
    }                                                                                              \
    {                                                                                              \
        /* ExecVP */                                                                               \
        run_exec_test(test_name, expected_return_value, expected_errno,                            \
            expected_exit_code, expected_output_regex, expected_output_regex_flags,                \
            ExecVP, file, NULL, file, __VA_ARGS__);                                                \
    }                                                                                              \
    {                                                                                              \
        /* ExecVPE */                                                                              \
        run_exec_test(test_name, expected_return_value, expected_errno,                            \
            expected_exit_code, expected_output_regex, expected_output_regex_flags,                \
            ExecVPE, file, envp, file, __VA_ARGS__);                                               \
    }                                                                                              \
                                                                                                   \
                                                                                                   \
    {                                                                                              \
        if (FEXECVE_SUPPORTED == 1) {                                                              \
            /* FExecVE */                                                                          \
            run_exec_test(test_name, expected_return_value, expected_errno,                        \
                expected_exit_code, expected_output_regex, expected_output_regex_flags,            \
                FExecVE, path, envp, path, __VA_ARGS__);                                           \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    } else ((void)0)





void testExec__Basic();
void testExec__Files(const char* current_path, char* env_current_path);
void testExec__PackageManager();

void runExecTests() {
    logDebug(LOG_TAG, "runExecTests()");

    const char* current_path = getenv(ENV__PATH);
    char* env_current_path = NULL;

    if (current_path == NULL || strlen(current_path) < 1) {
        env_current_path = ENV_PREFIX__PATH;
    } else {
        if (asprintf(&env_current_path, "%s%s", ENV_PREFIX__PATH, current_path) == -1) {
            errno = ENOMEM;
            logStrerrorDebug(LOG_TAG, "asprintf failed for current '%s%s'", ENV_PREFIX__PATH, current_path);
            exit(1);
        }
    }

    // TODO: Port tests from bionic
    // - https://cs.android.com/android/_/android/platform/bionic/+/refs/tags/android-14.0.0_r18:tests/unistd_test.cpp;l=1364
    // - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r18:bionic/tests/utils.h;l=200

    testExec__Basic();
    testExec__Files(current_path, env_current_path);
    testExec__PackageManager();

    int__AEqual(0, errno);

    // We cannot free this in a test method that sets is as later test cases will use it
    free(env_current_path);
}



void testExec__Basic() {
    logVerbose(LOG_TAG, "testExec__Basic()");

    run_all_exec_wrappers_test("rootfs",
        -1, EISDIR,
        0, NULL, 0,
        "../../", TERMUX__ROOTFS, environ,
        NULL);
}

void testExec__Files(const char* current_path, char* env_current_path) {
    logVerbose(LOG_TAG, "testExec__Files()");

    // execlp(), execvp() and execvpe() search for file to be executed in $PATH,
    // so set it with test exec files directory appended at end.
    char* env_new_path = NULL;

    if (current_path == NULL || strlen(current_path) < 1) {
        if (asprintf(&env_new_path, "%s%s", ENV_PREFIX__PATH, TERMUX_EXEC__EXEC_TEST_FILES_PATH) == -1) {
            errno = ENOMEM;
            logStrerrorDebug(LOG_TAG, "asprintf failed for new '%s%s'", ENV_PREFIX__PATH, TERMUX_EXEC__EXEC_TEST_FILES_PATH);
            exit(1);
        }
    } else {
        if (asprintf(&env_new_path, "%s%s:%s", ENV_PREFIX__PATH, current_path, TERMUX_EXEC__EXEC_TEST_FILES_PATH) == -1) {
            errno = ENOMEM;
            logStrerrorDebug(LOG_TAG, "asprintf failed for new '%s%s:%s'", ENV_PREFIX__PATH, current_path, TERMUX_EXEC__EXEC_TEST_FILES_PATH);
            exit(1);
        }
    }


    putenv(env_new_path);


    run_all_exec_wrappers_test("print-args-binary",
        0, 0,
        0, "^goodbye-world$", REG_EXTENDED,
        "print-args-binary", TERMUX_EXEC__EXEC_TEST_FILES_PATH "/print-args-binary", environ,
        "goodbye-world", NULL);

    run_all_exec_wrappers_test("print-args-binary.sym",
        0, 0,
        0, "^goodbye-world$", REG_EXTENDED,
        "print-args-binary.sym", TERMUX_EXEC__EXEC_TEST_FILES_PATH "/print-args-binary.sym", environ,
        "goodbye-world", NULL);


    run_all_exec_wrappers_test("print-args-linux-script.sh",
        0, 0,
        0, "^goodbye-world$", REG_EXTENDED,
        "print-args-linux-script.sh", TERMUX_EXEC__EXEC_TEST_FILES_PATH "/print-args-linux-script.sh", environ,
        "goodbye-world", NULL);

    run_all_exec_wrappers_test("print-args-linux-script.sh.sym",
        0, 0,
        0, "^goodbye-world$", REG_EXTENDED,
        "print-args-linux-script.sh.sym", TERMUX_EXEC__EXEC_TEST_FILES_PATH "/print-args-linux-script.sh.sym", environ,
        "goodbye-world", NULL);


    run_all_exec_wrappers_test("print-args-termux-script.sh",
        0, 0,
        0, "^goodbye-world$", REG_EXTENDED,
        "print-args-termux-script.sh", TERMUX_EXEC__EXEC_TEST_FILES_PATH "/print-args-termux-script.sh", environ,
        "goodbye-world", NULL);

    run_all_exec_wrappers_test("print-args-termux-script.sh.sym",
        0, 0,
        0, "^goodbye-world$", REG_EXTENDED,
        "print-args-termux-script.sh.sym", TERMUX_EXEC__EXEC_TEST_FILES_PATH "/print-args-termux-script.sh.sym", environ,
        "goodbye-world", NULL);


    putenv(env_current_path);

    free(env_new_path);

}

void testExec__PackageManager() {
    logVerbose(LOG_TAG, "testExec__PackageManager()");

    if (UID == 0) {
        logStrerrorVerbose(LOG_TAG, "Not running \"package-manager\" \"exec()\" wrapper tests since running as root");
        return;
    }

    char* termux_package_manager = getenv("TERMUX__PACKAGE_MANAGER");
    if (termux_package_manager == NULL || strlen(termux_package_manager) < 1) {
        logStrerrorVerbose(LOG_TAG, "Not running \"package-manager\" \"exec()\" wrapper tests since 'TERMUX__PACKAGE_MANAGER' environmental variable not set");
        return;
    }

    char* termux_package_manager_path = NULL;
    if (asprintf(&termux_package_manager_path, "%s/bin/%s", TERMUX__PREFIX, termux_package_manager) == -1) {
        errno = ENOMEM;
        logStrerrorDebug(LOG_TAG, "asprintf failed for new '%s/bin/%s'", TERMUX__PREFIX, termux_package_manager);
        exit(1);
    }

    // In case bootstrap was built without a package manager
    if (access(termux_package_manager_path, X_OK) != 0) {
        logStrerrorVerbose(LOG_TAG, "Not running \"package-manager\" \"exec()\" wrapper tests since failed to access package manager executable path '%s'",
            termux_package_manager_path);
        free(termux_package_manager_path);
        errno = 0;
        return;
    }



    // apt: `apt x.x.x (<arch>)`
    // pacman: `Pacman vx.x.x` Also can icon and license info
    char* termux_package_manager_version_regex = NULL;
    if (asprintf(&termux_package_manager_version_regex, "^.*%s v?[0-9][.][0-9][.][0-9].*$", termux_package_manager) == -1) {
        errno = ENOMEM;
        logStrerrorDebug(LOG_TAG, "asprintf failed for new '^.*%s v?[0-9][.][0-9][.][0-9].*$'", termux_package_manager);
        exit(1);
    }

    run_all_exec_wrappers_test("package-manager-version",
        0, 0,
        0, termux_package_manager_version_regex, REG_EXTENDED | REG_ICASE,
        termux_package_manager, termux_package_manager_path, environ,
        "--version", NULL);

    free(termux_package_manager_version_regex);



    free(termux_package_manager_path);

}






int main() {
    init();

    logInfo(LOG_TAG, "Start \"runtime_binary\" tests");

    runExecTests();

    logInfo(LOG_TAG, "End \"runtime_binary\" tests");

    return 0;
}



static void init() {
    errno = 0;

    UID = geteuid();

    initLogger();
}

static void initLogger() {
    setDefaultLogTagAndPrefix(TERMUX__NAME);
    setCurrentLogLevel(get_termux_exec_tests_log_level());
    setFormattedLogging(false);
}

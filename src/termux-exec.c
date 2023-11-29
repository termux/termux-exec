// Make android_get_device_api_level() use an inline variant,
// as a libc symbol for it exists only in android-29+.
#undef __ANDROID_API__
#define __ANDROID_API__ 28

#define _GNU_SOURCE
// #include <dlfcn.h>
// #include <string.h>
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

#if UINTPTR_MAX == 0xffffffff
#define SYSTEM_LINKER_PATH "/system/bin/linker";
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYSTEM_LINKER_PATH "/system/bin/linker64";
#endif

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

#define TERMUX_BASE_DIR_MAX_LEN 200

#define TERMUX_PREFIX_ENV_NAME "TERMUX__PREFIX"

#define LOG_PREFIX "[termux-exec] "

// Get the termux base directory, where all termux-installed packages reside under.
// This is normally /data/data/com.termux/files
static char const *get_termux_base_dir(char *buffer) {
  char const *prefix = getenv(TERMUX_PREFIX_ENV_NAME);
  if (prefix && strlen(prefix) <= TERMUX_BASE_DIR_MAX_LEN) {
    char *last_slash_ptr = strrchr(prefix, '/');
    if (last_slash_ptr != NULL) {
      size_t len_up_until_last_slash = last_slash_ptr - prefix + 1;
      if (len_up_until_last_slash > 10) { // Just a sanity check low limit.
        snprintf(buffer, len_up_until_last_slash, "%s", prefix);
        buffer[len_up_until_last_slash] = 0;
        return buffer;
      }
    }
  }
  return TERMUX_BASE_DIR;
}

// Check if `string` starts with `prefix`.
static bool starts_with(const char *string, const char *prefix) { return strncmp(string, prefix, strlen(prefix)) == 0; }

// Rewrite e.g. "/bin/sh" and "/usr/bin/sh" to "${TERMUX_PREFIX}/bin/sh".
//
// The termux_base_dir argument comes from get_termux_base_dir(), so have a
// max length of TERMUX_BASE_DIR_MAX_LEN.
static const char *termux_rewrite_executable(const char *termux_base_dir, const char *executable_path, char *buffer,
                                             int buffer_len) {
  char termux_bin_path[TERMUX_BASE_DIR_MAX_LEN + 16];
  int termux_bin_path_len = sprintf(termux_bin_path, "%s/usr/bin/", termux_base_dir);

  if (executable_path[0] != '/') {
    return executable_path;
  }

  char *bin_match = strstr(executable_path, "/bin/");
  if (bin_match == executable_path || bin_match == (executable_path + 4)) {
    // Found "/bin/" or "/xxx/bin" at the start of executable_path.
    strcpy(buffer, termux_bin_path);
    char *dest = buffer + termux_bin_path_len;
    // Copy what comes after "/bin/":
    const char *src = bin_match + 5;
    size_t bytes_to_copy = buffer_len - termux_bin_path_len;
    strncpy(dest, src, bytes_to_copy);
    return buffer;
  } else {
    return executable_path;
  }
}

// Remove LD_LIBRARY_PATH and LD_LIBRARY_PATH entries from envp.
static void remove_ld_from_env(char *const *envp, char ***allocation) {
  bool create_new_env = false;
  int env_length = 0;
  while (envp[env_length] != NULL) {
    if (starts_with(envp[env_length], "LD_LIBRARY_PATH=") || starts_with(envp[env_length], "LD_PRELOAD=")) {
      create_new_env = true;
    }
    env_length++;
  }

  if (!create_new_env) {
    return;
  }

  char **new_envp = malloc(sizeof(char *) * env_length);
  *allocation = new_envp;
  int new_envp_idx = 0;
  int old_envp_idx = 0;

  while (old_envp_idx < env_length) {
    if (!starts_with(envp[old_envp_idx], "LD_LIBRARY_PATH=") && !starts_with(envp[old_envp_idx], "LD_PRELOAD=")) {
      new_envp[new_envp_idx++] = envp[old_envp_idx];
    }
    old_envp_idx++;
  }

  new_envp[new_envp_idx] = NULL;
}

// From https://stackoverflow.com/questions/4774116/realpath-without-resolving-symlinks/34202207#34202207
static const char *normalize_path(const char *src, char *result_buffer) {
  char pwd[PATH_MAX];
  if (getcwd(pwd, sizeof(pwd)) == NULL) {
    return src;
  }

  size_t res_len;
  size_t src_len = strlen(src);

  const char *ptr = src;
  const char *end = &src[src_len];
  const char *next;

  if (src_len == 0 || src[0] != '/') {
    // relative path
    size_t pwd_len = strlen(pwd);
    memcpy(result_buffer, pwd, pwd_len);
    res_len = pwd_len;
  } else {
    res_len = 0;
  }

  for (ptr = src; ptr < end; ptr = next + 1) {
    next = (char *)memchr(ptr, '/', end - ptr);
    if (next == NULL) {
      next = end;
    }
    size_t len = next - ptr;
    switch (len) {
    case 2:
      if (ptr[0] == '.' && ptr[1] == '.') {
        const char *slash = (char *)memrchr(result_buffer, '/', res_len);
        if (slash != NULL) {
          res_len = slash - result_buffer;
        }
        continue;
      }
      break;
    case 1:
      if (ptr[0] == '.') {
        continue;
      }
      break;
    case 0:
      continue;
    }

    if (res_len != 1) {
      result_buffer[res_len++] = '/';
    }

    memcpy(&result_buffer[res_len], ptr, len);
    res_len += len;
  }

  if (res_len == 0) {
    result_buffer[res_len++] = '/';
  }
  result_buffer[res_len] = '\0';
  return result_buffer;
}

struct file_header_info {
  bool is_elf;
  // If executing a 32-bit binary on a 64-bit host:
  bool is_non_native_elf;
  char interpreter_buf[256];
  char const *interpreter;
  char const *interpreter_arg;
};

static void inspect_file_header(char const *termux_base_dir, char *header, size_t header_len,
                                struct file_header_info *result) {
  if (header_len >= 20 && !memcmp(header, ELFMAG, SELFMAG)) {
    result->is_elf = true;
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)header;
    if (ehdr->e_machine != EM_NATIVE) {
      result->is_non_native_elf = true;
    }
    return;
  }

  if (header_len < 5 || !(header[0] == '#' && header[1] == '!')) {
    return;
  }

  // Check if the header contains a newline to end the shebang line:
  char *newline_location = memchr(header, '\n', header_len);
  if (newline_location == NULL) {
    return;
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
    return;
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
      result->interpreter_arg = interpreter_arg;
    }
  }

  result->interpreter =
      termux_rewrite_executable(termux_base_dir, interpreter, result->interpreter_buf, sizeof(result->interpreter_buf));
}

static int execve_syscall(const char *executable_path, char *const argv[], char *const envp[]) {
  return syscall(SYS_execve, executable_path, argv, envp);
}

// Interceptor of the execve(2) system call using LD_PRELOAD.
int execve(const char *executable_path, char *const argv[], char *const envp[]) {
  if (getenv("TERMUX_EXEC_OPTOUT") != NULL) {
    return execve_syscall(executable_path, argv, envp);
  }

  const bool termux_exec_debug = getenv("TERMUX_EXEC_DEBUG") != NULL;
  if (termux_exec_debug) {
    fprintf(stderr, LOG_PREFIX "Intercepting execve('%s'):\n", executable_path);
    int tmp_argv_count = 0;
    while (argv[tmp_argv_count] != NULL) {
      fprintf(stderr, LOG_PREFIX "   argv[%d] = '%s'\n", tmp_argv_count, argv[tmp_argv_count]);
      tmp_argv_count++;
    }
  }

  // Size of TERMUX_BASE_DIR_MAX_LEN + "/usr/bin/sh".
  char termux_base_dir_buf[TERMUX_BASE_DIR_MAX_LEN + 16];

  const char *termux_base_dir = get_termux_base_dir(termux_base_dir_buf);
  if (termux_exec_debug) {
    fprintf(stderr, LOG_PREFIX "Using base dir: '%s'\n", termux_base_dir);
  }

  const char *orig_executable_path = executable_path;

  char executable_path_buffer[PATH_MAX];
  executable_path = termux_rewrite_executable(termux_base_dir, executable_path, executable_path_buffer,
                                              sizeof(executable_path_buffer));
  if (termux_exec_debug && executable_path_buffer == executable_path) {
    fprintf(stderr, LOG_PREFIX "Rewritten path: '%s'\n", executable_path);
  }

  if (access(executable_path, X_OK) != 0) {
    // Error out if the file is not executable:
    errno = EACCES;
    return -1;
  }

  int fd = open(executable_path, O_RDONLY);
  if (fd == -1) {
    errno = ENOENT;
    return -1;
  }

  // execve(2): "The kernel imposes a maximum length on the text that follows the "#!" characters
  // at the start of a script; characters beyond the limit are ignored. Before Linux 5.1, the
  // limit is 127 characters. Since Linux 5.1, the limit is 255 characters."
  // We use one more byte since inspect_file_header() will null terminate the buffer.
  char header[256];
  ssize_t read_bytes = read(fd, header, sizeof(header) - 1);
  close(fd);

  struct file_header_info info = {
      .interpreter = NULL,
      .interpreter_arg = NULL,
  };
  inspect_file_header(termux_base_dir, header, read_bytes, &info);

  if (!info.is_elf && info.interpreter == NULL) {
    if (termux_exec_debug) {
      fprintf(stderr, LOG_PREFIX "Not ELF or shebang, returning ENOEXEC\n");
    }
    errno = ENOEXEC;
    return -1;
  }

  if (info.interpreter != NULL) {
    executable_path = info.interpreter;
    if (termux_exec_debug) {
      fprintf(stderr, LOG_PREFIX "Path to interpreter from shebang: '%s'\n", info.interpreter);
    }
  }

  char normalized_path_buffer[PATH_MAX];
  executable_path = normalize_path(executable_path, normalized_path_buffer);

  char **new_allocated_envp = NULL;

  const char **new_argv = NULL;

  // Only wrap with linker:
  // - If running on an Android 10+ where it's necesssary, and
  // - If trying to execute a file under the termux base directory
  bool wrap_in_linker =
#ifdef __ANDROID__
      android_get_device_api_level() >= 29 && android_get_application_target_sdk_version() >= 29 &&
#endif
      (strstr(executable_path, termux_base_dir) != NULL);

  bool cleanup_env = info.is_non_native_elf ||
                     // Avoid interfering with Android /system software by removing
                     // LD_PRELOAD and LD_LIBRARY_PATH from env if executing something
                     // there. But /system/bin/{sh,linker,linker64} are ok with those
                     // and should keep them.
                     (starts_with(executable_path, "/system/") && !starts_with(executable_path, "/system/bin/sh") &&
                      !starts_with(executable_path, "/system/bin/linker"));
  if (cleanup_env) {
    remove_ld_from_env(envp, &new_allocated_envp);
    if (new_allocated_envp) {
      envp = new_allocated_envp;
      if (termux_exec_debug) {
        fprintf(stderr, LOG_PREFIX "Removed LD_PRELOAD and LD_LIBRARY_PATH from env\n");
      }
    }
  }

  const bool argv_needs_rewriting = wrap_in_linker || info.interpreter != NULL;
  if (argv_needs_rewriting) {
    int orig_argv_count = 0;
    while (argv[orig_argv_count] != NULL) {
      orig_argv_count++;
    }

    new_argv = malloc(sizeof(char *) * (2 + orig_argv_count));
    int current_argc = 0;

    // Keep program name:
    new_argv[current_argc++] = argv[0];

    // Specify executable path if wrapping with linker:
    if (wrap_in_linker) {
      // Normalize path without resolving symlink. For instance, $PREFIX/bin/ls is
      // a symlink to $PREFIX/bin/coreutils, but we need to execute
      // "/system/bin/linker $PREFIX/bin/ls" so that coreutils knows what to execute.
      new_argv[current_argc++] = executable_path;
      executable_path = SYSTEM_LINKER_PATH;
    }

    // Add interpreter argument and script path if exec:ing a script with shebang:
    if (info.interpreter != NULL) {
      if (info.interpreter_arg) {
        new_argv[current_argc++] = info.interpreter_arg;
      }
      new_argv[current_argc++] = orig_executable_path;
    }

    for (int i = 1; i < orig_argv_count; i++) {
      new_argv[current_argc++] = argv[i];
    }
    new_argv[current_argc] = NULL;
    argv = (char **)new_argv;
  }

  if (termux_exec_debug) {
    fprintf(stderr, LOG_PREFIX "Calling syscall execve('%s'):\n", executable_path);
    int tmp_argv_count = 0;
    int arg_count = 0;
    while (argv[tmp_argv_count] != NULL) {
      fprintf(stderr, LOG_PREFIX "   argv[%d] = '%s'\n", arg_count++, argv[tmp_argv_count]);
      tmp_argv_count++;
    }
  }

  // Setup TERMUX_EXEC__PROC_SELF_EXE as the absolute path to the executable
  // file we are executing. That is, we are executing:
  //    /system/bin/linker64 ${TERMUX_EXEC__PROC_SELF_EXE} ..arguments..
  // This is to allow patching software that uses /proc/self/exe
  // to instead use getenv("TERMUX_EXEC__PROC_SELF_EXE").
  char *termux_self_exe;
  if (asprintf(&termux_self_exe, "TERMUX_EXEC__PROC_SELF_EXE=%s", orig_executable_path) == -1) {
    if (termux_exec_debug) {
      fprintf(stderr, LOG_PREFIX "asprintf failed\n");
    }
    free(new_argv);
    free(new_allocated_envp);
    errno = ENOMEM;
    return -1;
  }
  int num_env = 0;
  while (envp[num_env] != NULL) {
    num_env++;
  }
  // Allocate new environment variable array. Size + 2 since
  // we might perhaps append a TERMUX_EXEC__PROC_SELF_EXE variable and
  // we will also NULL terminate.
  char **new_envp = malloc(sizeof(char *) * (num_env + 2));
  bool already_found_self_exe = false;
  for (int i = 0; i < num_env; i++) {
    if (starts_with(envp[i], "TERMUX_EXEC__PROC_SELF_EXE=")) {
      new_envp[i] = termux_self_exe;
      already_found_self_exe = true;
      if (termux_exec_debug) {
        fprintf(stderr, LOG_PREFIX "Overwriting to '%s'\n", termux_self_exe);
      }
    } else {
      new_envp[i] = envp[i];
    }
  }
  if (!already_found_self_exe) {
    new_envp[num_env++] = termux_self_exe;
    if (termux_exec_debug) {
      fprintf(stderr, LOG_PREFIX "Setting new '%s'\n", termux_self_exe);
    }
  }
  // Null terminate.
  new_envp[num_env] = NULL;

  int syscall_ret = execve_syscall(executable_path, argv, new_envp);
  int saved_errno = errno;
  if (termux_exec_debug) {
    perror(LOG_PREFIX "execve() syscall failed");
  }
  free(new_argv);
  free(new_allocated_envp);
  free(new_envp);
  free(termux_self_exe);
  errno = saved_errno;
  return syscall_ret;
}

#ifdef UNIT_TEST
#include <assert.h>
#define TERMUX_BIN_PATH TERMUX_BASE_DIR "/usr/bin/"

void assert_string_equals(const char *expected, const char *actual) {
  if (strcmp(actual, expected) != 0) {
    fprintf(stderr, "Assertion failed - expected '%s', was '%s'\n", expected, actual);
    exit(1);
  }
}

void test_starts_with() {
  assert(starts_with("/path/to/file", "/path"));
  assert(!starts_with("/path", "/path/to/file"));
}

void test_termux_rewrite_executable() {
  char buf[PATH_MAX];
  char const *b = TERMUX_BASE_DIR;
  assert_string_equals(TERMUX_BIN_PATH "sh", termux_rewrite_executable(b, "/bin/sh", buf, PATH_MAX));
  assert_string_equals(TERMUX_BIN_PATH "sh", termux_rewrite_executable(b, "/usr/bin/sh", buf, PATH_MAX));
  assert_string_equals("/system/bin/sh", termux_rewrite_executable(b, "/system/bin/sh", buf, PATH_MAX));
  assert_string_equals("/system/bin/tool", termux_rewrite_executable(b, "/system/bin/tool", buf, PATH_MAX));
  assert_string_equals(TERMUX_BIN_PATH "sh", termux_rewrite_executable(b, TERMUX_BIN_PATH "sh", buf, PATH_MAX));
  assert_string_equals(TERMUX_BIN_PATH, termux_rewrite_executable(b, "/bin/", buf, PATH_MAX));
  assert_string_equals("./ab/sh", termux_rewrite_executable(b, "./ab/sh", buf, PATH_MAX));
}

void test_remove_ld_from_env() {
  {
    char *test_env[] = {"MY_ENV=1", NULL};
    char **allocated_envp;
    remove_ld_from_env(test_env, &allocated_envp);
    assert(allocated_envp == NULL);
    assert_string_equals("MY_ENV=1", test_env[0]);
    assert(test_env[1] == NULL);
  }
  {
    char *test_env[] = {"MY_ENV=1", "LD_PRELOAD=a", NULL};
    char **allocated_envp;
    remove_ld_from_env(test_env, &allocated_envp);
    assert(allocated_envp != NULL);
    assert_string_equals("MY_ENV=1", allocated_envp[0]);
    assert(allocated_envp[1] == NULL);
    free(allocated_envp);
  }
  {
    char *test_env[] = {"MY_ENV=1", "LD_PRELOAD=a", "A=B", "LD_LIBRARY_PATH=B", "B=C", NULL};
    char **allocated_envp;
    remove_ld_from_env(test_env, &allocated_envp);
    assert(allocated_envp != NULL);
    assert_string_equals("MY_ENV=1", allocated_envp[0]);
    assert_string_equals("A=B", allocated_envp[1]);
    assert_string_equals("B=C", allocated_envp[2]);
    assert(allocated_envp[3] == NULL);
    free(allocated_envp);
  }
}

void test_normalize_path() {
  char expected[PATH_MAX * 2];
  char pwd[PATH_MAX];
  char normalized_path_buffer[PATH_MAX];

  assert(getcwd(pwd, sizeof(pwd)) != NULL);

  sprintf(expected, "%s/path/to/binary", pwd);
  assert_string_equals(expected, normalize_path("path/to/binary", normalized_path_buffer));
  assert_string_equals(expected, normalize_path("path/../path/to/binary", normalized_path_buffer));
  assert_string_equals(expected, normalize_path("./path/to/../to/binary", normalized_path_buffer));
  assert_string_equals(
      "/usr/bin/sh", normalize_path("../../../../../../../../../../../../usr/./bin/../bin/sh", normalized_path_buffer));
}

void test_inspect_file_header() {
  char header[256];
  struct file_header_info info = {.interpreter_arg = NULL};

  sprintf(header, "#!/bin/sh\n");
  inspect_file_header(TERMUX_BASE_DIR, header, 256, &info);
  assert(!info.is_elf);
  assert(!info.is_non_native_elf);
  assert_string_equals(TERMUX_BIN_PATH "sh", info.interpreter);
  assert(info.interpreter_arg == NULL);

  sprintf(header, "#!/bin/sh -x\n");
  inspect_file_header(TERMUX_BASE_DIR, header, 256, &info);
  assert(!info.is_elf);
  assert(!info.is_non_native_elf);
  assert_string_equals(TERMUX_BIN_PATH "sh", info.interpreter);
  assert_string_equals("-x", info.interpreter_arg);

  sprintf(header, "#! /bin/sh -x\n");
  inspect_file_header(TERMUX_BASE_DIR, header, 256, &info);
  assert(!info.is_elf);
  assert(!info.is_non_native_elf);
  assert_string_equals(TERMUX_BIN_PATH "sh", info.interpreter);
  assert_string_equals("-x", info.interpreter_arg);

  sprintf(header, "#!/bin/sh -x \n");
  inspect_file_header(TERMUX_BASE_DIR, header, 256, &info);
  assert(!info.is_elf);
  assert(!info.is_non_native_elf);
  assert_string_equals(TERMUX_BIN_PATH "sh", info.interpreter);
  assert_string_equals("-x", info.interpreter_arg);

  sprintf(header, "#!/bin/sh -x \n");
  inspect_file_header(TERMUX_BASE_DIR, header, 256, &info);
  assert(!info.is_elf);
  assert(!info.is_non_native_elf);
  assert_string_equals(TERMUX_BIN_PATH "sh", info.interpreter);
  assert_string_equals("-x", info.interpreter_arg);

  info.interpreter = NULL;
  info.interpreter_arg = NULL;
  // An ELF header for a 32-bit file.
  // See https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#File_header
  sprintf(header, "\177ELF");
  // Native instruction set:
  header[0x12] = EM_NATIVE;
  header[0x13] = 0;
  inspect_file_header(TERMUX_BASE_DIR, header, 256, &info);
  assert(info.is_elf);
  assert(!info.is_non_native_elf);
  assert(info.interpreter == NULL);
  assert(info.interpreter_arg == NULL);

  info.interpreter = NULL;
  info.interpreter_arg = NULL;
  // An ELF header for a 64-bit file.
  // See https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#File_header
  sprintf(header, "\177ELF");
  // 'Fujitsu MMA Multimedia Accelerator' instruction set - likely non-native.
  header[0x12] = 0x36;
  header[0x13] = 0;
  inspect_file_header(TERMUX_BASE_DIR, header, 256, &info);
  assert(info.is_elf);
  assert(info.is_non_native_elf);
  assert(info.interpreter == NULL);
  assert(info.interpreter_arg == NULL);
}

void test_get_termux_base_dir() {
  char buffer[TERMUX_BASE_DIR_MAX_LEN + 16];

  setenv(TERMUX_PREFIX_ENV_NAME, "/data/data/com.termux/files/usr", true);
  assert_string_equals("/data/data/com.termux/files", get_termux_base_dir(buffer));

  setenv(TERMUX_PREFIX_ENV_NAME, "/data/data/com.termux/files/usr/made/up", true);
  assert_string_equals("/data/data/com.termux/files/usr/made", get_termux_base_dir(buffer));

  setenv(TERMUX_PREFIX_ENV_NAME, "/a/path/to/some/prefix/here", true);
  assert_string_equals("/a/path/to/some/prefix", get_termux_base_dir(buffer));

  setenv(TERMUX_PREFIX_ENV_NAME, "/a/path/to/some/prefix/here", true);
  assert_string_equals("/a/path/to/some/prefix", get_termux_base_dir(buffer));

  unsetenv(TERMUX_PREFIX_ENV_NAME);
  assert_string_equals(TERMUX_BASE_DIR, get_termux_base_dir(buffer));
}

int main() {
  test_starts_with();
  test_termux_rewrite_executable();
  test_remove_ld_from_env();
  test_normalize_path();
  test_inspect_file_header();
  test_get_termux_base_dir();
  return 0;
}
#endif

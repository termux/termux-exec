#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char* termux_rewrite_executable(const char* filename, char* buffer, int buffer_len)
{
	strcpy(buffer, "/data/data/com.termux/files/usr/bin/");
	char* bin_match = strstr(filename, "/bin/");
	if (bin_match == filename || bin_match == (filename + 4)) {
		// We have either found "/bin/" at the start of the string or at
		// "/xxx/bin/". Take the path after that.
		strncpy(buffer + 36, bin_match + 5, buffer_len - 37);
		filename = buffer;
	}
	return filename;
}

int execve(const char* filename, char* const* argv, char *const envp[])
{
	bool android_10_debug = getenv("TERMUX_ANDROID10_DEBUG") != NULL;
	if (android_10_debug) {
		printf("execve(%s):\n", filename);
		int tmp_argv_count = 0;
		while (argv[tmp_argv_count] != NULL) {
			printf("  %s\n", argv[tmp_argv_count]);
			tmp_argv_count++;
		}
	}

	int fd = -1;
	const char** new_argv = NULL;

	char filename_buffer[512];
	filename = termux_rewrite_executable(filename, filename_buffer, sizeof(filename_buffer));

	// Error out if the file is not executable:
	if (access(filename, X_OK) != 0) goto final;

	fd = open(filename, O_RDONLY);
	if (fd == -1) goto final;

	// execve(2): "A maximum line length of 127 characters is allowed
	// for the first line in a #! executable shell script."
	char shebang[128];
	ssize_t read_bytes = read(fd, shebang, sizeof(shebang) - 1);
	if (read_bytes < 5 || !(shebang[0] == '#' && shebang[1] == '!')) goto final;

	shebang[read_bytes] = 0;
	char* newline_location = strchr(shebang, '\n');
	if (newline_location == NULL) goto final;

	// Strip whitespace at end of shebang:
	while (*(newline_location - 1) == ' ') newline_location--;

	// Null-terminate the shebang line:
	*newline_location = 0;

	// Skip whitespace to find interpreter start:
	char* interpreter = shebang + 2;
	while (*interpreter == ' ') interpreter++;
	if (interpreter == newline_location) goto final;

	char* arg = NULL;
	char* whitespace_pos = strchr(interpreter, ' ');
	if (whitespace_pos != NULL) {
		// Null-terminate the interpreter string.
		*whitespace_pos = 0;

		// Find start of argument:
		arg = whitespace_pos + 1;
		while (*arg != 0 && *arg == ' ') arg++;
		if (arg == newline_location) {
			// Only whitespace after interpreter.
			arg = NULL;
		}
	}

	char interp_buf[512];
	const char* new_interpreter = termux_rewrite_executable(interpreter, interp_buf, sizeof(interp_buf));
	if (new_interpreter == interpreter) goto final;

	int orig_argv_count = 0;
	while (argv[orig_argv_count] != NULL) orig_argv_count++;

	new_argv = malloc(sizeof(char*) * (4 + orig_argv_count));

	int current_argc = 0;
	new_argv[current_argc++] = basename(interpreter);
	if (arg) new_argv[current_argc++] = arg;
	new_argv[current_argc++] = filename;
	int i = 1;
	while (orig_argv_count-- > 1) new_argv[current_argc++] = argv[i++];
	new_argv[current_argc] = NULL;

	filename = new_interpreter;
	argv = (char**) new_argv;

final:
	if (fd != -1) close(fd);
	int (*real_execve)(const char*, char *const[], char *const[]) = dlsym(RTLD_NEXT, "execve");

	bool android_10_wrapping = getenv("TERMUX_ANDROID10") != NULL;
	if (android_10_wrapping) {
		char realpath_buffer[PATH_MAX];
		bool realpath_ok = realpath(filename, realpath_buffer) != NULL;
		if (realpath_ok) {
			bool wrap_in_proot = (strstr(realpath_buffer, "/data/data/com.termux/files") != NULL);
			if (android_10_debug) {
				printf("termux-exec: realpath(\"%s\") = \"%s\", wrapping=%s\n", filename, realpath_buffer, wrap_in_proot ? "yes" : "no");
			}
			if (wrap_in_proot) {
				orig_argv_count = 0;
				while (argv[orig_argv_count] != NULL) orig_argv_count++;

				new_argv = malloc(sizeof(char*) * (2 + orig_argv_count));
				filename = "/data/data/com.termux/files/usr/bin/proot";
				new_argv[0] = "proot";
				for (int i = 0; i < orig_argv_count; i++) {
					new_argv[i + 1] = argv[i];
				}
				new_argv[orig_argv_count + 1] = NULL;
				argv = (char**) new_argv;

				// Remove LD_PRELOAD environment variable when wrapping in proot:
				for (int i = 0; envp[i] != NULL; i++) {
					if (strstr(envp[i], "LD_PRELOAD=") == envp[i]) {
						int env_length = 0;
						while (envp[env_length] != NULL) env_length++;

						char** new_envp = malloc(sizeof(char*) * env_length);
						int new_envp_idx = 0;
						int old_envp_idx = 0;
						while (old_envp_idx < env_length) {
							if (old_envp_idx != i) {
								new_envp[new_envp_idx++] = envp[old_envp_idx];
							}
							old_envp_idx++;
						}
						new_envp[env_length] = NULL;
						envp = new_envp;
						break;
					}
				}
			}
		} else {
			errno = 0;
		}

		if (android_10_debug) {
			printf("real_execve(%s):\n", filename);
			int tmp_argv_count = 0;
			while (argv[tmp_argv_count] != NULL) {
				printf("  %s\n", argv[tmp_argv_count]);
				tmp_argv_count++;
			}
		}
	}

	int ret = real_execve(filename, argv, envp);
	free(new_argv);
	return ret;
}

#define _GNU_SOURCE
// for RTLD_NEXT
// ref: http://comments.gmane.org/gmane.linux.man/5208

#include "termux_rewrite_executable.h"

#include <dlfcn.h>

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int execve(const char* filename, char* const* argv, char *const envp[])
{
	int fd = -1;
	const char** new_argv = NULL;

	char filename_buffer[512];
	filename = termux_rewrite_executable(filename_buffer, sizeof(filename_buffer), filename);

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
	const char* new_interpreter =
		termux_rewrite_executable(interp_buf, sizeof(interp_buf), interpreter);
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
	int ret = real_execve(filename, argv, envp);
	free(new_argv);
	return ret;
}

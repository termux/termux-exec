/*
  command line test driver

	## command ##
  $ ./termux-rewrite-exe /bin/sh /usr/bin/env
	## output ##
  /data/data/com.termux/files/usr/bin/sh
  /data/data/com.termux/files/usr/bin/env
*/

#include "termux_rewrite_executable.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

int main(int argc, char** argv) {
	int verbose = 0;
	size_t idx = 1;
	for ( ; idx < argc; ++idx ) {
		if (strcmp("-v", argv[idx]) == 0) {
		  verbose = 1;
			continue;
		}
		if (strcmp("--", argv[idx]) == 0) {
			++idx;
			break;
		}
		if (strncmp("-", argv[idx], 1) == 0) {
			fprintf(stderr, "unrecognized option '%s'", argv[idx]);
			return 1;
		}

		break;
	}
	const char* fmt[]={
		"%.0s%s\n",
		"'%s' -> '%s'\n"
	};

	char rewritten[512];
	for( ; idx < argc; ++idx ) {
		printf(
			fmt[verbose],
			argv[idx],
			termux_rewrite_executable(rewritten, sizeof(rewritten), argv[idx]));
	}
	return 0;
}

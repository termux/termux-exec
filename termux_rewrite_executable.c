#include "termux_rewrite_executable.h"

#include <string.h>
#include <stdio.h>

const char* termux_rewrite_executable(
	char* rewritten, size_t rewritten_size, const char* original)
{
	const char* bin_match = strstr(original, "/bin/");
	// We have either found "/bin/" at the start of the string or at
	// "/xxx/bin/". Take the path after that.
	if (bin_match != original && bin_match != (original + 4)) {
		snprintf(rewritten, rewritten_size, "%s", original);
		return rewritten;
	}

	const char termux_usr_bin[]="/data/data/com.termux/files/usr/bin";
	snprintf(rewritten, rewritten_size, "%s/%s", termux_usr_bin, bin_match + 5);
	return rewritten;
}

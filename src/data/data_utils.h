#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include <stdbool.h>

/** Check if `str` starts with `prefix`. */
bool string_starts_with(const char *string, const char *prefix);

/** Check if `str` ends with `suffix`. */
bool string_ends_with(const char *string, const char *suffix);



int string_to_int(const char* string, int def, const char* log_tag, const char *fmt, ...);



/**
 * Check if `string` matches the `pattern` regex.
 *
 * - https://man7.org/linux/man-pages/man0/regex.h.0p.html
 * - https://pubs.opengroup.org/onlinepubs/009696899/functions/regcomp.html
 *
 * @param string The string to match.
 * @param pattern The regex pattern to match with.
 * @param cflags The flags for `regcomp()` like `REG_EXTENDED`.
 * @return Returns `0` on match, `1` on no match and `-1` on other failures.
 */
int regex_match(const char *string, const char *pattern, int cflags);

#endif // DATA_UTILS_H

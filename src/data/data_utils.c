#define _GNU_SOURCE
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../logger/logger.h"
#include "data_utils.h"

static const char* LOG_TAG = "data_utils";

bool string_starts_with(const char *string, const char *prefix) {
    if (string == NULL || prefix == NULL) {
        return false;
    }

    return *string != '\0' && *prefix != '\0' && (strncmp(string, prefix, strlen(prefix)) == 0);
}

bool string_ends_with(const char *string, const char *suffix) {
    if (string == NULL || suffix == NULL) {
        return false;
    }

    int strLength = strlen(string);
    int suffixLength = strlen(suffix);

    return *string != '\0' && *suffix != '\0' && (strLength >= suffixLength) && \
        (strcmp(string + (strLength - suffixLength), suffix) == 0);
}



int string_to_int(const char* string, int def, const char* log_tag, const char *fmt, ...) {
    (void)log_tag;
    (void)fmt;

    // We do not use atoi as it will not set errno or return errors.
    // - https://man7.org/linux/man-pages/man3/atoi.3.html
    // - https://man7.org/linux/man-pages/man3/strtol.3.html
    char *end;
    errno = 0;
    long value = strtol(string, &end, 10);
    if (end == string || *end != '\0' || errno != 0 || value < 0 || value > INT_MAX) {
        #ifdef TERMUX_EXEC__RUNNING_TESTS
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            logStrerrorMessage(errno, log_tag, fmt, args);
            va_end(args);
        }
        #endif
        errno = 0;
        return def;
    }

    return (int) value;
}



int regex_match(const char *string, const char *pattern, int cflags) {
    if (string == NULL || *string == '\0') {
        return 1;
    }

    regex_t regex;
    int result;
    char msgbuf[100];

    // Compile regular expression
    result = regcomp(&regex, pattern, cflags);
    if (result != 0) {
        regerror(result, &regex, msgbuf, sizeof(msgbuf));
        logErrorDebug(LOG_TAG, "Failed to compile regex '%s': %s", pattern, msgbuf);
        return -1;
    }

    // Match regular expression
    result = regexec(&regex, string, 0, NULL, 0);
    int match;
    if (result == 0) {
        match = 0;
    } else if (result == REG_NOMATCH) {
        match = 1;
    } else {
        regerror(result, &regex, msgbuf, sizeof(msgbuf));
        logErrorDebug(LOG_TAG, "Regex match failed '%s': %s", pattern, msgbuf);
        match =-1;
    }

    regfree(&regex);

    return match;
}

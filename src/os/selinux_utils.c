#define _GNU_SOURCE
#include <errno.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../data/data_utils.h"
#include "../logger/logger.h"
#include "selinux_utils.h"

bool get_se_process_context_from_env(const char* log_tag, const char* var, char buffer[], size_t buffer_len) {
    const char* value = getenv(var);
    if (value == NULL || strlen(value) < 1) {
        return false;
    }

    if (regex_match(value, REGEX__PROCESS_CONTEXT, REG_EXTENDED) != 0) {
        logErrorVVerbose(log_tag, "Ignoring invalid se_process_context value set in '%s' env variable: '%s'",
            var, value);
        return false;
    }

    strncpy(buffer, value, buffer_len);
    buffer[buffer_len - 1] = '\0';

    return true;
}

bool get_se_process_context_from_file(const char* log_tag, char buffer[], size_t buffer_len) {
    FILE *fp = fopen("/proc/self/attr/current", "r");
    if (fp == NULL) {
        logErrorVVerbose(log_tag, "Failed to open '/proc/self/attr/current' to read se_process_context: '%d'", errno);
        return false;
    }

    if (fgets(buffer, buffer_len, fp) != NULL) {
        if (buffer_len > 0 && buffer[buffer_len-1] == '\n') {
            buffer[--buffer_len] = '\0';
        }
    } else {
        logErrorVVerbose(log_tag, "Failed to read se_process_context from '/proc/self/attr/current': '%d'", errno);
        return false;
    }

    fclose(fp);
    return true;
}

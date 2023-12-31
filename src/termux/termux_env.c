#define _GNU_SOURCE
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../data/data_utils.h"
#include "../logger/logger.h"
#include "termux_env.h"

static const char* LOG_TAG = "termux_env";

bool get_bool_env_value(const char *name, bool def) {
    const char* value = getenv(name);
    if (value == NULL || strlen(value) < 1) {
        return def;
    } else if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
               strcmp(value, "on") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "y") == 0) {
        return true;
    } else if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 ||
               strcmp(value, "off") == 0 || strcmp(value, "no") == 0 || strcmp(value, "n") == 0) {
        return false;
    }
    return def;
}



bool are_vars_in_env(char *const *envp, const char *vars[], int n) {
    int env_length = 0;
    while (envp[env_length] != NULL) {
        for (int i = 0; i < n; i++) {
            if (string_starts_with(envp[env_length], vars[i])) {
                return true;
            }
        }
        env_length++;
    }
    return false;
}





static int get_log_level_value(const char *name) {
    const char* value = getenv(name);
    if (value == NULL || strlen(value) < 1) {
        return DEFAULT_LOG_LEVEL;
    } else {
        char log_level_string[strlen(value) + 1];
        strncpy(log_level_string, value, sizeof(log_level_string));
        int log_level = string_to_int(log_level_string, -1,
            LOG_TAG, "Failed to convert '%s' env variable value '%s' to an int", name, log_level_string);
        if (log_level < 0) {
            return DEFAULT_LOG_LEVEL;
        }
        return log_level;
    }
}

int get_termux_exec_log_level() {
    return get_log_level_value(ENV__TERMUX_EXEC__LOG_LEVEL);
}



bool is_termux_exec_execve_intercept_enabled() {
    return get_bool_env_value(ENV__TERMUX_EXEC__INTERCEPT_EXECVE, E_DEF_VAL__TERMUX_EXEC__INTERCEPT_EXECVE);
}

int get_termux_exec_system_linker_exec_config() {
    int def = E_DEF_VAL__TERMUX_EXEC__SYSTEM_LINKER_EXEC;
    const char* value = getenv(ENV__TERMUX_EXEC__SYSTEM_LINKER_EXEC);
    if (value == NULL || strlen(value) < 1) {
        return def;
    } else if (strcmp(value, "disable") == 0) {
        return 0;
    } else if (strcmp(value, "enable") == 0) {
        return 1;
    } else if (strcmp(value, "force") == 0) {
        return 2;
    }
    return def;
}



int get_termux_exec_tests_log_level() {
    return get_log_level_value(ENV__TERMUX_EXEC_TESTS__LOG_LEVEL);
}

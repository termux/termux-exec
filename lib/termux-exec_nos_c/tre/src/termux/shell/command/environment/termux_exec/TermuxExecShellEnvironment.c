#define _GNU_SOURCE
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <termux/termux_core__nos__c/v1/logger/Logger.h>
#include <termux/termux_core__nos__c/v1/termux/shell/command/environment/TermuxShellEnvironment.h>
#include <termux/termux_core__nos__c/v1/unix/shell/command/environment/UnixShellEnvironment.h>

#include <termux/termux_exec__nos__c/v1/termux/shell/command/environment/termux_exec/TermuxExecShellEnvironment.h>



int termuxExec_logLevel_get() {
    return getLogLevelFromEnv(ENV__TERMUX_EXEC__LOG_LEVEL);
}



int termuxExec_execveCall_intercept_get() {
    int def = ENV_DEF_VAL__TERMUX_EXEC__EXECVE_CALL__INTERCEPT;
    const char* value = getenv(ENV__TERMUX_EXEC__EXECVE_CALL__INTERCEPT);
    if (value == NULL || strlen(value) < 1) {
        return def;
    } else if (strcmp(value, "disable") == 0) {
        return 0;
    } else if (strcmp(value, "enable") == 0) {
        return 1;
    }
    return def;
}

int termuxExec_systemLinkerExec_mode_get() {
    int def = ENV_DEF_VAL__TERMUX_EXEC__SYSTEM_LINKER_EXEC__MODE;
    const char* value = getenv(ENV__TERMUX_EXEC__SYSTEM_LINKER_EXEC__MODE);
    if (value == NULL || strlen(value) < 1) {
        return def;
    } else if (strcmp(value, "disable") == 0) {
        return 0;
    } else if (strcmp(value, "enable") == 0) {
        return 1;
    } else if (strcmp(value, "force") == 0) {
        return 2;
    } else if (strcmp(value, "force_all") == 0) {
        return 3;
    }
    return def;
}



int termuxExec_tests_logLevel_get() {
    return getLogLevelFromEnv(ENV__TERMUX_EXEC__TESTS__LOG_LEVEL);
}

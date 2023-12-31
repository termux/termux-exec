#define _GNU_SOURCE
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "assert_utils.h"

void assert_bool_with_error(bool expected, bool actual, const char* log_tag, const char *fmt, ...) {
    if (actual != expected) {
        logError(log_tag, "FAILED: expected: %s, actual: %s", expected ? "true" : "false", actual ? "true" : "false");
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            logMessage(LOG_PRIORITY_ERROR, log_tag, fmt, args);
            va_end(args);
        }
        exit(1);
    }
}

void assert_bool(bool expected, bool actual, const char* log_tag) {
    assert_bool_with_error(expected, actual, log_tag, NULL);
}



void assert_false_with_error(bool actual, const char* log_tag, const char *fmt, ...) {
    if (actual != false) {
        logError(log_tag, "FAILED: expected: false, actual: true");
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            logMessage(LOG_PRIORITY_ERROR, log_tag, fmt, args);
            va_end(args);
        }
        exit(1);
    }
}

void assert_false(bool actual, const char* log_tag) {
    assert_false_with_error(actual, log_tag, NULL);
}



void assert_true_with_error(bool actual, const char* log_tag, const char *fmt, ...) {
    if (actual != true) {
        logError(log_tag, "FAILED: expected: true, actual: false");
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            logMessage(LOG_PRIORITY_ERROR, log_tag, fmt, args);
            va_end(args);
        }
        exit(1);
    }
}

void assert_true(bool actual, const char* log_tag) {
    assert_true_with_error(actual, log_tag, NULL);
}





void assert_int_with_error(int expected, int actual, const char* log_tag, const char *fmt, ...) {
    if (actual != expected) {
        logError(log_tag, "FAILED: expected: %d, actual: %d", expected, actual);
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            logMessage(LOG_PRIORITY_ERROR, log_tag, fmt, args);
            va_end(args);
        }
        exit(1);
    }
}

void assert_int(int expected, int actual, const char* log_tag) {
    assert_int_with_error(expected, actual, log_tag, NULL);
}





bool string_equals_regarding_null(const char *expected, const char *actual) {
    if (expected == NULL) {
        return actual == NULL;
    }

    return actual != NULL && strcmp(actual, expected) == 0;
}


void assert_string_equals_with_error(const char *expected, const char *actual,
    const char* log_tag, const char *fmt, ...) {
    if (!string_equals_regarding_null(expected, actual)) {
        if (expected == NULL) {
            logError(log_tag, "FAILED: expected: null, actual: '%s'", actual);
        } else if (actual == NULL) {
            logError(log_tag, "FAILED: expected: '%s', actual: null", expected);
        } else {
            logError(log_tag, "FAILED: expected: '%s', actual: '%s'", expected, actual);
        }
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            logMessage(LOG_PRIORITY_ERROR, log_tag, fmt, args);
            va_end(args);
        }
        exit(1);
    }
}

void assert_string_equals(const char *expected, const char *actual, const char* log_tag) {
    assert_string_equals_with_error(expected, actual, log_tag, NULL);
}




void assert_string_null_with_error(const char *actual, const char* log_tag, const char *fmt, ...) {
    if (actual != NULL) {
        logError(log_tag, "FAILED: expected: null, actual: '%s'", actual);
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            logMessage(LOG_PRIORITY_ERROR, log_tag, fmt, args);
            va_end(args);
        }
        exit(1);
    }
}

void assert_string_null(const char *actual, const char* log_tag) {
    assert_string_null_with_error(actual, log_tag, NULL);
}



void assert_string_not_null_with_error(const char *actual, const char* log_tag, const char *fmt, ...) {
    if (actual == NULL) {
        logError(log_tag, "FAILED: expected: not_null, actual: '%s'", actual);
        if (fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            logMessage(LOG_PRIORITY_ERROR, log_tag, fmt, args);
            va_end(args);
        }
        exit(1);
    }
}

void assert_string_not_null(const char *actual, const char* log_tag) {
    assert_string_not_null_with_error(actual, log_tag, NULL);
}

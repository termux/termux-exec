#ifndef ASSERT_UTILS_H
#define ASSERT_UTILS_H

#include <stdbool.h>

#include "../logger/logger.h"

void assert_bool_with_error(bool expected, bool actual, const char* log_tag, const char *fmt, ...);
void assert_bool(bool expected, bool actual, const char* log_tag);



void assert_true_with_error(bool actual, const char* log_tag, const char *error_fmt, ...);
void assert_true(bool actual, const char* log_tag);



void assert_false_with_error(bool actual, const char* log_tag, const char *fmt, ...);
void assert_false(bool actual, const char* log_tag);





void assert_int_with_error(int expected, int actual, const char* log_tag, const char *fmt, ...);
void assert_int(int expected, int actual, const char* log_tag);





bool string_equals_regarding_null(const char *expected, const char *actual);

void assert_string_equals_with_error(const char *expected, const char *actual,
    const char* log_tag, const char *fmt, ...);
void assert_string_equals(const char *expected, const char *actual, const char* log_tag);



void assert_string_null_with_error(const char *actual, const char* log_tag, const char *fmt, ...);
void assert_string_null(const char *actual, const char* log_tag);



void assert_string_not_null_with_error(const char *actual, const char* log_tag, const char *fmt, ...);
void assert_string_not_null(const char *actual, const char* log_tag);





#define state__ATrue(state)                                                                        \
    if (1) {                                                                                       \
    assert_true_with_error(state,                                                                  \
        LOG_TAG, "%d: %s() -> (%s)", __LINE__, __FUNCTION__, #state);                              \
    } else ((void)0)

#define int__AEqual(expected, actual)                                                              \
    if (1) {                                                                                       \
    assert_int_with_error(expected, actual,                                                        \
        LOG_TAG, "%d: %s()", __LINE__, __FUNCTION__);                                              \
    } else ((void)0)

#define string__AEqual(expected, actual)                                                           \
    if (1) {                                                                                       \
    assert_string_equals_with_error(expected, actual,                                              \
        LOG_TAG, "%d: %s()", __LINE__, __FUNCTION__);                                              \
    } else ((void)0)

#endif // ASSERT_UTILS_H

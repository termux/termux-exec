#define _GNU_SOURCE
#include <assert.h>
#include <elf.h>
#include <fcntl.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/data/assert_utils.h"
#include "../src/data/data_utils.h"
#include "../src/exec/exec.h"
#include "../src/file/file_utils.h"
#include "../src/logger/logger.h"
#include "../src/termux/termux_env.h"
#include "../src/termux/termux_files.h"

static const char* LOG_TAG = "unit_tests";

static void init();
static void initLogger();



void testStringStartsWith__Basic();

void runStringStartsWithTests() {
    logDebug(LOG_TAG, "runStringStartsWithTests()");

    testStringStartsWith__Basic();

    int__AEqual(0, errno);
}



#define ssw__ATrue(string, prefix)                                                                 \
    if (1) {                                                                                       \
    assert_true_with_error(string_starts_with(string, prefix),                                     \
        LOG_TAG, "%d: string_starts_with('%s', '%s')", __LINE__, string , prefix);                 \
    assert_int_with_error(0, errno,                                                        \
        LOG_TAG, "%d: %s()", __LINE__, __FUNCTION__);                                              \
    } else ((void)0)

#define ssw__AFalse(string, prefix)                                                                \
    if (1) {                                                                                       \
    assert_false_with_error(string_starts_with(string, prefix),                                    \
        LOG_TAG, "%d: string_starts_with('%s', '%s')", __LINE__, string , prefix);                 \
    assert_int_with_error(0, errno,                                                        \
        LOG_TAG, "%d: %s()", __LINE__, __FUNCTION__);                                              \
    } else ((void)0)



void testStringStartsWith__Basic() {
    logVerbose(LOG_TAG, "testStringStartsWith__Basic()");

    ssw__AFalse(NULL, "/path");
    ssw__AFalse("/path", NULL);
    ssw__AFalse(NULL, NULL);
    ssw__AFalse("", "");
    ssw__AFalse("/", "");
    ssw__AFalse("", "/");
    ssw__AFalse("", " ");
    ssw__AFalse(" ", "");
    ssw__ATrue(" ", " ");
    ssw__AFalse("", "\0");
    ssw__AFalse("\0", "");
    ssw__AFalse("\0", "\0");
    ssw__AFalse("\0", "a");
    ssw__AFalse("a", "\0");

    ssw__ATrue("/path/to/file", "/path");
    ssw__AFalse("/path", "/path/to/file");

    ssw__ATrue("/path", "/");
    ssw__ATrue("/path", "/p");
    ssw__ATrue("/path/to/file", "/");
    ssw__ATrue("/path/to/file", "/p");
    ssw__ATrue("/path/to/file", "/path/");
    ssw__ATrue("/path/to/file", "/path/to");
    ssw__ATrue("/path/to/file", "/path/to/");
    ssw__ATrue("/path/to/file", "/path/to/file");
    ssw__AFalse("/path/to/file", "/path/to/file/");

}





void testStringEndsWith__Basic();

void runStringEndsWithTests() {
    logDebug(LOG_TAG, "runStringEndsWithTests()");

    testStringEndsWith__Basic();

    int__AEqual(0, errno);
}



#define sew__ATrue(string, prefix)                                                                 \
    if (1) {                                                                                       \
    assert_true_with_error(string_ends_with(string, prefix),                                       \
        LOG_TAG, "%d: string_ends_with('%s', '%s')", __LINE__, string , prefix);                   \
    } else ((void)0)

#define sew__AFalse(string, prefix)                                                                \
    if (1) {                                                                                       \
    assert_false_with_error(string_ends_with(string, prefix),                                      \
        LOG_TAG, "%d: string_ends_with('%s', '%s')", __LINE__, string , prefix);                   \
    } else ((void)0)



void testStringEndsWith__Basic() {
    logVerbose(LOG_TAG, "testStringEndsWith__Basic()");

    sew__AFalse(NULL, "/path");
    sew__AFalse("/path", NULL);
    sew__AFalse(NULL, NULL);
    sew__AFalse("", "");
    sew__AFalse("/", "");
    sew__AFalse("", "/");
    sew__AFalse("", " ");
    sew__AFalse(" ", "");
    sew__ATrue(" ", " ");
    sew__AFalse("", "\0");
    sew__AFalse("\0", "");
    sew__AFalse("\0", "\0");
    sew__AFalse("\0", "a");
    sew__AFalse("a", "\0");

    sew__ATrue("/path/to/file", "/file");
    sew__AFalse("/file", "/path/to/file");

    sew__ATrue("/path", "h");
    sew__ATrue("/path", "/path");
    sew__AFalse("/path", "hh");
    sew__ATrue("/path/to/file", "/file");
    sew__ATrue("/path/to/file", "to/file");
    sew__ATrue("/path/to/file", "/to/file");
    sew__ATrue("/path/to/file", "path/to/file");
    sew__ATrue("/path/to/file", "/path/to/file");

}





void testAbsolutizePath__Basic();

void runAbsolutizePathTests() {
    logDebug(LOG_TAG, "runAbsolutizePathTests()");

    testAbsolutizePath__Basic();

    int__AEqual(0, errno);
}



#define absolutizeP__AEqual(expected_path, path)                                                   \
    if (1) {                                                                                       \
    char absolute_path[PATH_MAX];                                                                  \
    assert_string_equals_with_error(expected_path, absolutize_path(path, absolute_path, sizeof(absolute_path)), \
    LOG_TAG, "%d: absolutize_path('%s')", __LINE__, path);                                         \
    errno = 0;                                                                                     \
    } else ((void)0)

#define absolutizeP__ANull(path)                                                                   \
    if (1) {                                                                                       \
    char absolute_path[PATH_MAX];                                                                  \
    assert_string_null_with_error(absolutize_path(path, absolute_path, sizeof(absolute_path)),     \
    LOG_TAG, "%d: absolutize_path('%s')", __LINE__, path);                                         \
    errno = 0;                                                                                     \
    } else ((void)0)



void testAbsolutizePath__Basic() {
    logVerbose(LOG_TAG, "testAbsolutizePath__Basic()");

    char expected[PATH_MAX];
    char cwd[PATH_MAX];

    state__ATrue(getcwd(cwd, sizeof(cwd)) != NULL);

    // Ensure enough space for 25 char binary under TERMUX_BIN_DIR for tests
    state__ATrue(strlen(cwd) + strlen(TERMUX_BIN_DIR "/xxxxxxxxxxxxxxxxxxxxxxxxx") < PATH_MAX);


    absolutizeP__ANull(NULL);
    absolutizeP__ANull("");

    {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/%0*d", PATH_MAX - 2, 0);
    absolutizeP__AEqual(path, path);
    }

    {
    char path[PATH_MAX + 1];
    snprintf(path, sizeof(path), "/%0*d", PATH_MAX - 1, 0);
    absolutizeP__ANull(path);
    }

    {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%0*d", PATH_MAX - 1, 0);
    absolutizeP__ANull(path);
    }

    {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%0*d", PATH_MAX - 2, 0);
    state__ATrue(strlen(cwd) >= 1);
    absolutizeP__ANull(path);
    }


    snprintf(expected, sizeof(expected), "%s/path/to/binary", cwd);
    absolutizeP__AEqual(expected, "path/to/binary");

    snprintf(expected, sizeof(expected), "%s/path/to/../binary", cwd);
    absolutizeP__AEqual(expected, "path/to/../binary");


    snprintf(expected, sizeof(expected), "%s/../path/to/binary", cwd);
    absolutizeP__AEqual(expected, "../path/to/binary");

    snprintf(expected, sizeof(expected), "%s/../../path/to/binary", cwd);
    absolutizeP__AEqual(expected, "../../path/to/binary");


    snprintf(expected, sizeof(expected), "%s/./path/to/binary", cwd);
    absolutizeP__AEqual(expected, "./path/to/binary");

    snprintf(expected, sizeof(expected), "%s/././path/to/binary", cwd);
    absolutizeP__AEqual(expected, "././path/to/binary");


    snprintf(expected, sizeof(expected), "%s/.././path/to/binary", cwd);
    absolutizeP__AEqual(expected, ".././path/to/binary");

    snprintf(expected, sizeof(expected), "%s/./../path/to/binary", cwd);
    absolutizeP__AEqual(expected, "./../path/to/binary");


    {
    char path[PATH_MAX];
    snprintf(expected, sizeof(expected), "%s/path/to/binary", cwd);
    snprintf(path, sizeof(path), "%s/path/to/binary", cwd);
    absolutizeP__AEqual(expected, path);
    }


    {
    char path[PATH_MAX];
    snprintf(expected, sizeof(expected), "%s/../path/to/binary", cwd);
    snprintf(path, sizeof(path), "%s/../path/to/binary", cwd);
    absolutizeP__AEqual(expected, path);
    }


    snprintf(expected, sizeof(expected), "%s/binary", cwd);
    absolutizeP__AEqual(expected, "binary");

    {
    char path[PATH_MAX];
    snprintf(expected, sizeof(expected), "%s///binary", cwd);
    snprintf(path, sizeof(path), "%s///binary", cwd);
    absolutizeP__AEqual(expected, path);
    }

}





void testNormalizePath__KeepEndSeparator();
void testNormalizePath__NoEndSeparator();
void testNormalizePath__SingleDots();
void testNormalizePath__DoubleDots();
void testNormalizePath__FromMethodDocs();
void testNormalizePath__BinaryPaths();

/**
 * Test normalization of file paths.
 *
 * Most of the tests were taken and modified from following under Apache-2.0 license.
 * - https://github.com/apache/commons-io/blob/51894ebab6d260962caacd9e33394226bbf891c4/src/test/java/org/apache/commons/io/FilenameUtilsTest.java#L758
 * - https://github.com/apache/commons-io/blob/master/LICENSE.txt
 */
void runNormalizePathTests() {
    logDebug(LOG_TAG, "runNormalizePathTests()");

    testNormalizePath__KeepEndSeparator();
    testNormalizePath__NoEndSeparator();
    testNormalizePath__SingleDots();
    testNormalizePath__DoubleDots();
    testNormalizePath__FromMethodDocs();
    testNormalizePath__BinaryPaths();

    int__AEqual(0, errno);
}



#define normalizeP__AEqual(expected_path, path_param, keepEndSeparator, removeDoubleDot)           \
    if (1) {                                                                                       \
    const char* path = path_param;                                                                 \
    if (path != NULL) {                                                                            \
        char input_path[PATH_MAX];                                                                 \
        strncpy(input_path, path, sizeof(input_path) - 1);                                         \
        assert_string_equals_with_error(expected_path, normalize_path(input_path, keepEndSeparator, removeDoubleDot), \
        LOG_TAG, "%d: normalize_path('%s', %s, %s)", __LINE__, input_path, #keepEndSeparator, #removeDoubleDot); \
    } else {                                                                                       \
        assert_string_equals_with_error(expected_path, normalize_path(NULL, keepEndSeparator, removeDoubleDot), \
        LOG_TAG, "%d: normalize_path(null, %s, %s)", __LINE__, #keepEndSeparator, #removeDoubleDot); \
    }                                                                                              \
    } else ((void)0)

#define normalizeP__ANull(path_param, keepEndSeparator, removeDoubleDot)                           \
    if (1) {                                                                                       \
    const char* path = path_param;                                                                 \
    if (path != NULL) {                                                                            \
        char input_path[PATH_MAX];                                                                 \
        strncpy(input_path, path, sizeof(input_path) - 1);                                         \
        assert_string_null_with_error(normalize_path(input_path, keepEndSeparator, removeDoubleDot), \
        LOG_TAG, "%d: normalize_path('%s', %s, %s)", __LINE__, input_path, #keepEndSeparator, #removeDoubleDot); \
    } else {                                                                                       \
        assert_string_null_with_error(normalize_path(NULL, keepEndSeparator, removeDoubleDot),     \
        LOG_TAG, "%d: normalize_path(null, %s, %s)", __LINE__, #keepEndSeparator, #removeDoubleDot); \
    }                                                                                              \
    } else ((void)0)

#define normalizeP__AEqual__KES(expected, path) normalizeP__AEqual(expected, path, true, true)
#define normalizeP__ANull__KES(path) normalizeP__ANull(path, true, true)

#define normalizeP__AEqual__NES(expected, path) normalizeP__AEqual(expected, path, false, true)
#define normalizeP__ANull__NES(path) normalizeP__ANull(path, false, true)



void testNormalizePath__KeepEndSeparator() {
    logVerbose(LOG_TAG, "testNormalizePath__KeepEndSeparator()");

    normalizeP__ANull(NULL, true, true);
    normalizeP__ANull__KES("");

    normalizeP__AEqual__KES("/", "/");
    normalizeP__AEqual__KES("/", "//");
    normalizeP__AEqual__KES("/", "///");
    normalizeP__AEqual__KES("/", "////");

    normalizeP__AEqual__KES("/a", "/a");
    normalizeP__AEqual__KES("/a", "//a");
    normalizeP__AEqual__KES("/a/", "//a/");
    normalizeP__AEqual__KES("/a/", "//a//");
    normalizeP__AEqual__KES("/a/", "//a///");
    normalizeP__AEqual__KES("/a/", "///a///");
    normalizeP__AEqual__KES("/a/b", "///a/b");
    normalizeP__AEqual__KES("/a/b/", "///a/b/");
    normalizeP__AEqual__KES("/a/b/", "///a/b//");
    normalizeP__AEqual__KES("/a/b/", "///a//b//");
    normalizeP__AEqual__KES("/a/b/", "///a//b///");
    normalizeP__AEqual__KES("/a/b/", "///a///b///");
    normalizeP__AEqual__KES("a/", "a//");
    normalizeP__AEqual__KES("a/", "a///");
    normalizeP__AEqual__KES("a/", "a///");
    normalizeP__AEqual__KES("a/b", "a/b");
    normalizeP__AEqual__KES("a/b/", "a/b/");
    normalizeP__AEqual__KES("a/b/", "a/b//");
    normalizeP__AEqual__KES("a/b/", "a//b//");
    normalizeP__AEqual__KES("a/b/", "a//b///");
    normalizeP__AEqual__KES("a/b/", "a///b///");
    normalizeP__AEqual__KES("a/b/c", "a/b//c");
    normalizeP__AEqual__KES("/a/b/c", "/a/b//c");
    normalizeP__AEqual__KES("/a/b/c/", "/a/b//c/");
    normalizeP__AEqual__KES("/a/b/c/d/", "///a//b///c/d///");
    normalizeP__AEqual__KES("/a/b/c/d", "///a//b///c/d");
    normalizeP__AEqual__KES("/a/b/c/d/", "//a//b///c/d//");

    normalizeP__AEqual__KES("~", "~");
    normalizeP__AEqual__KES("~/", "~/");
    normalizeP__AEqual__KES("~/a", "~/a");
    normalizeP__AEqual__KES("~/a/", "~/a/");
    normalizeP__AEqual__KES("~/a/b/d", "~/a/b//d");
    normalizeP__AEqual__KES("~user/a", "~user/a");
    normalizeP__AEqual__KES("~user/a/", "~user/a/");
    normalizeP__AEqual__KES("~user/a/b/d", "~user/a/b//d");
    normalizeP__AEqual__KES("~user/", "~user/");
    normalizeP__AEqual__KES("~user", "~user");
    normalizeP__AEqual__KES("~user/a/b/c.txt", "~user/a/b/c.txt");

    normalizeP__AEqual__KES("C:/a", "C:/a");
    normalizeP__AEqual__KES("C:/a/", "C:/a/");
    normalizeP__AEqual__KES("C:/a/b/d", "C:/a/b//d");
    normalizeP__AEqual__KES("C:/", "C:/");
    normalizeP__AEqual__KES("C:a", "C:a");
    normalizeP__AEqual__KES("C:a/", "C:a/");
    normalizeP__AEqual__KES("C:a/b/d", "C:a/b//d");
    normalizeP__AEqual__KES("C:", "C:");
    normalizeP__AEqual__KES("C:/a/b/c.txt", "C:/a/b/c.txt");

    normalizeP__AEqual__KES("/server/a", "//server/a");
    normalizeP__AEqual__KES("/server/a/", "//server/a/");
    normalizeP__AEqual__KES("/server/a/b/d", "//server/a/b//d");
    normalizeP__AEqual__KES("/server/", "//server/");
    normalizeP__AEqual__KES("/server/a/b/c.txt", "//server/a/b/c.txt");
    normalizeP__AEqual__KES("/-server/a/b/c.txt", "/-server/a/b/c.txt");

    normalizeP__AEqual__KES(":", ":");
    normalizeP__AEqual__KES("1:", "1:");
    normalizeP__AEqual__KES("1:a", "1:a");
    normalizeP__AEqual__KES("1:/a/b/c.txt", "1:/a/b/c.txt");
    normalizeP__AEqual__KES("/a/b/c.txt", "///a/b/c.txt");
    normalizeP__AEqual__KES("a/b/c.txt", "a/b/c.txt");
    normalizeP__AEqual__KES("/a/b/c.txt", "/a/b/c.txt");
    normalizeP__AEqual__KES("~/a/b/c.txt", "~/a/b/c.txt");
    normalizeP__AEqual__KES("/127.0.0.1/a/b/c.txt", "//127.0.0.1/a/b/c.txt");
    normalizeP__AEqual__KES("/::1/a/b/c.txt", "//::1/a/b/c.txt");
    normalizeP__AEqual__KES("/1::/a/b/c.txt", "//1::/a/b/c.txt");
    normalizeP__AEqual__KES("/server.example.org/a/b/c.txt", "//server.example.org/a/b/c.txt");
    normalizeP__AEqual__KES("/server.sub.example.org/a/b/c.txt", "/server.sub.example.org/a/b/c.txt");
    normalizeP__AEqual__KES("/server./a/b/c.txt", "//server./a/b/c.txt");
    normalizeP__AEqual__KES("/1::127.0.0.1/a/b/c.txt", "//1::127.0.0.1/a/b/c.txt");

    // Not a valid IPv4 addresses but technically a valid "reg-name"s according to RFC1034
    normalizeP__AEqual__KES("/127.0.0.256/a/b/c.txt", "//127.0.0.256/a/b/c.txt");
    normalizeP__AEqual__KES("/127.0.0.01/a/b/c.txt", "//127.0.0.01/a/b/c.txt");

    normalizeP__AEqual__KES("/::1::2/a/b/c.txt", "//::1::2/a/b/c.txt");
    normalizeP__AEqual__KES("/:1/a/b/c.txt", "//:1/a/b/c.txt");
    normalizeP__AEqual__KES("/1:/a/b/c.txt", "//1:/a/b/c.txt");
    normalizeP__AEqual__KES("/1:2:3:4:5:6:7:8:9/a/b/c.txt", "//1:2:3:4:5:6:7:8:9/a/b/c.txt");
    normalizeP__AEqual__KES("/g:2:3:4:5:6:7:8/a/b/c.txt", "//g:2:3:4:5:6:7:8/a/b/c.txt");
    normalizeP__AEqual__KES("/1ffff:2:3:4:5:6:7:8/a/b/c.txt", "//1ffff:2:3:4:5:6:7:8/a/b/c.txt");
    normalizeP__AEqual__KES("/1:2/a/b/c.txt", "//1:2/a/b/c.txt");
}

void testNormalizePath__NoEndSeparator() {
    logVerbose(LOG_TAG, "testNormalizePath__NoEndSeparator()");

    normalizeP__ANull(NULL, false, true);
    normalizeP__ANull__NES("");

    normalizeP__AEqual__NES("/", "/");
    normalizeP__AEqual__NES("/", "//");
    normalizeP__AEqual__NES("/", "///");
    normalizeP__AEqual__NES("/", "////");

    normalizeP__AEqual__NES("/a", "/a");
    normalizeP__AEqual__NES("/a", "//a");
    normalizeP__AEqual__NES("/a", "//a/");
    normalizeP__AEqual__NES("/a", "//a//");
    normalizeP__AEqual__NES("/a", "//a///");
    normalizeP__AEqual__NES("/a", "///a///");
    normalizeP__AEqual__NES("/a/b", "///a/b");
    normalizeP__AEqual__NES("/a/b", "///a/b/");
    normalizeP__AEqual__NES("/a/b", "///a/b//");
    normalizeP__AEqual__NES("/a/b", "///a//b//");
    normalizeP__AEqual__NES("/a/b", "///a//b///");
    normalizeP__AEqual__NES("/a/b", "///a///b///");
    normalizeP__AEqual__NES("a", "a//");
    normalizeP__AEqual__NES("a", "a///");
    normalizeP__AEqual__NES("a", "a///");
    normalizeP__AEqual__NES("a/b", "a/b");
    normalizeP__AEqual__NES("a/b", "a/b/");
    normalizeP__AEqual__NES("a/b", "a/b//");
    normalizeP__AEqual__NES("a/b", "a//b//");
    normalizeP__AEqual__NES("a/b", "a//b///");
    normalizeP__AEqual__NES("a/b", "a///b///");
    normalizeP__AEqual__NES("a/b/c", "a/b//c");
    normalizeP__AEqual__NES("/a/b/c", "/a/b//c");
    normalizeP__AEqual__NES("/a/b/c", "/a/b//c/");
    normalizeP__AEqual__NES("/a/b/c/d", "///a//b///c/d///");
    normalizeP__AEqual__NES("/a/b/c/d", "///a//b///c/d");
    normalizeP__AEqual__NES("/a/b/c/d", "//a//b///c/d//");

    normalizeP__AEqual__NES("~", "~");
    normalizeP__AEqual__NES("~", "~/");
    normalizeP__AEqual__NES("~user/a", "~user/a");
    normalizeP__AEqual__NES("~user/a", "~user/a/");
    normalizeP__AEqual__NES("~user/a/b/d", "~user/a/b//d/");
    normalizeP__AEqual__NES("~user", "~user/");
    normalizeP__AEqual__NES("~user", "~user");
    normalizeP__AEqual__NES("~/a", "~/a");
    normalizeP__AEqual__NES("~/a", "~/a/");
    normalizeP__AEqual__NES("~/a/b/d", "~/a/b//d/");
    normalizeP__AEqual__NES("~/a/b/c.txt", "~/a/b/c.txt/");
    normalizeP__AEqual__NES("~user/a/b/c.txt", "~user/a/b/c.txt/");

    normalizeP__AEqual__NES("C:/a", "C:/a");
    normalizeP__AEqual__NES("C:/a", "C:/a/");
    normalizeP__AEqual__NES("C:/a/b/d", "C:/a/b//d/");
    normalizeP__AEqual__NES("C:", "C://");
    normalizeP__AEqual__NES("C:a", "C:a");
    normalizeP__AEqual__NES("C:a", "C:a/");
    normalizeP__AEqual__NES("C:a/b/d", "C:a/b//d/");
    normalizeP__AEqual__NES("C:", "C:/");
    normalizeP__AEqual__NES("C:/a/b/c.txt", "C:/a/b/c.txt/");
    normalizeP__AEqual__NES("C:/a/b/c.txt", "C://a//b//c.txt/");

    normalizeP__AEqual__NES("/server/a", "//server/a");
    normalizeP__AEqual__NES("/server/a", "//server/a/");
    normalizeP__AEqual__NES("/server/a/b/d", "//server/a/b//d/");
    normalizeP__AEqual__NES("/server", "//server/");
    normalizeP__AEqual__NES("/server/a/b/c.txt", "//server/a/b/c.txt/");

    normalizeP__AEqual__NES(":", ":");
    normalizeP__AEqual__NES("1:", "1:");
    normalizeP__AEqual__NES("1:a", "1:a");
    normalizeP__AEqual__NES("1:/a/b/c.txt", "1:/a/b/c.txt");
    normalizeP__AEqual__NES("a/b/c.txt", "a/b/c.txt/");
    normalizeP__AEqual__NES("/a/b/c.txt", "/a/b/c.txt/");
    normalizeP__AEqual__NES("/a/b/c.txt", "///a/b/c.txt");
}

void testNormalizePath__SingleDots() {
    logVerbose(LOG_TAG, "testNormalizePath__SingleDots()");

    normalizeP__ANull__KES(".");
    normalizeP__ANull__KES("./");
    normalizeP__AEqual__KES("/", "/.");
    normalizeP__AEqual__KES("/", "/./");
    normalizeP__ANull__KES("././");
    normalizeP__AEqual__KES("/", "/./.");

    normalizeP__AEqual__KES("/a", "/././a");
    normalizeP__AEqual__KES("/b/", "/./b/./");
    normalizeP__AEqual__KES("/b", "/./b/.");
    normalizeP__AEqual__KES("c/", "./c/./");
    normalizeP__AEqual__KES("c", "./c/.");
    normalizeP__AEqual__KES("c/", "././c/./");
    normalizeP__AEqual__KES("c", "././c/.");
    normalizeP__AEqual__KES("c/", "././c/././");
    normalizeP__AEqual__KES("c", "././c/./.");
    normalizeP__AEqual__KES("a/b", "a/b/././.");
    normalizeP__AEqual__KES("a/b/", "a/b/./././");
    normalizeP__AEqual__KES("a/", "./a/");
    normalizeP__AEqual__KES("a", "./a");
    normalizeP__AEqual__KES("/a/b/", "/a/b/./././");
    normalizeP__AEqual__KES("/a/b", "/a/b/././.");
    normalizeP__AEqual__KES("/a", "/./a");
    normalizeP__AEqual__KES("/", "/./");
    normalizeP__AEqual__KES("/", "/.");

    normalizeP__AEqual__KES("~/a/b/", "~/a/b/./././");
    normalizeP__AEqual__KES("~/a/b", "~/a/b/././.");
    normalizeP__AEqual__KES("~/a", "~/./a");
    normalizeP__AEqual__KES("~/", "~/./");
    normalizeP__AEqual__KES("~", "~/.");
    normalizeP__AEqual__KES("~/", "~/");
    normalizeP__AEqual__KES("~user/a/b/", "~user/a/b/./././");
    normalizeP__AEqual__KES("~user/a/b", "~user/a/b/././.");
    normalizeP__AEqual__KES("~user/a", "~user/./a");
    normalizeP__AEqual__KES("~user/", "~user/./");
    normalizeP__AEqual__KES("~user", "~user/.");

    normalizeP__AEqual__KES("C:/a/b/", "C:/a/b/./././");
    normalizeP__AEqual__KES("C:/a/b", "C:/a/b/././.");
    normalizeP__AEqual__KES("C:/a", "C:/./a");
    normalizeP__AEqual__KES("C:/", "C:/./");
    normalizeP__AEqual__KES("C:", "C:/.");
    normalizeP__AEqual__KES("C:a/b/", "C:a/b/./././");
    normalizeP__AEqual__KES("C:a/b", "C:a/b/././.");
    normalizeP__AEqual__KES("C:./a", "C:./a");
    normalizeP__AEqual__KES("C:./", "C:./");
    normalizeP__AEqual__KES("C:.", "C:.");

    normalizeP__AEqual__KES("/server/a/b/", "//server/a/b/./././");
    normalizeP__AEqual__KES("/server/a/b", "//server/a/b/././.");
    normalizeP__AEqual__KES("/server/a", "//server/./a");
    normalizeP__AEqual__KES("/server/", "//server/./");
    normalizeP__AEqual__KES("/server", "//server/.");

    normalizeP__AEqual__KES("/a/b/c.txt", "/./a/b/c.txt");
    normalizeP__AEqual__KES("/127.0.0.256/a/b/c.txt", "//./127.0.0.256/./a/b/c.txt");



    normalizeP__ANull__NES(".");
    normalizeP__ANull__NES("./");
    normalizeP__AEqual__NES("/", "/.");
    normalizeP__AEqual__NES("/", "/./");
    normalizeP__ANull__NES("././");
    normalizeP__AEqual__NES("/", "/./.");

    normalizeP__AEqual__NES("/a", "/././a/");
    normalizeP__AEqual__NES("/b", "/./b/./");
    normalizeP__AEqual__NES("/b", "/./b/.");
    normalizeP__AEqual__NES("c", "./c/./");
    normalizeP__AEqual__NES("c", "./c/.");
    normalizeP__AEqual__NES("c", "././c/./");
    normalizeP__AEqual__NES("c", "././c/.");
    normalizeP__AEqual__NES("c", "././c/././");
    normalizeP__AEqual__NES("c", "././c/./.");
    normalizeP__AEqual__NES("/", "/./.");
    normalizeP__AEqual__NES("/", "/.");
    normalizeP__AEqual__NES("a/b", "a/b/./././");
    normalizeP__AEqual__NES("a/b", "a/b/./././/");
    normalizeP__AEqual__NES("a", "./a//");
    normalizeP__AEqual__NES("a", "./a/");
    normalizeP__AEqual__NES("/a/b", "/a/b/./././");
    normalizeP__AEqual__NES("/a/b", "/a/b/./././");
    normalizeP__AEqual__NES("/a", "/./a/");
    normalizeP__AEqual__NES("/", "/./");
    normalizeP__AEqual__NES("/", "/./");

    normalizeP__AEqual__NES("~/a/b", "~/a/b/./././");
    normalizeP__AEqual__NES("~/a/b", "~/a/b/./././");
    normalizeP__AEqual__NES("~/a", "~/./a/");
    normalizeP__AEqual__NES("~", "~/./");
    normalizeP__AEqual__NES("~", "~/./");
    normalizeP__AEqual__NES("~user/a/b", "~user/a/b/./././");
    normalizeP__AEqual__NES("~user/a/b", "~user/a/b/././.");
    normalizeP__AEqual__NES("~user/a", "~user/./a/");
    normalizeP__AEqual__NES("~user", "~user/./");
    normalizeP__AEqual__NES("~user", "~user/./");

    normalizeP__AEqual__NES("C:/a/b", "C:/a/b/./././");
    normalizeP__AEqual__NES("C:/a/b", "C:/a/b/././.");
    normalizeP__AEqual__NES("C:/a", "C:/./a/");
    normalizeP__AEqual__NES("C:", "C:/./");
    normalizeP__AEqual__NES("C:", "C:/./");
    normalizeP__AEqual__NES("C:a/b", "C:a/b/./././");
    normalizeP__AEqual__NES("C:a/b", "C:a/b/././.");
    normalizeP__AEqual__NES("C:./a", "C:./a/");
    normalizeP__AEqual__NES("C:.", "C:./");
    normalizeP__AEqual__NES("C:.", "C:.");

    normalizeP__AEqual__NES("/server/a/b", "//server/a/b/./././");
    normalizeP__AEqual__NES("/server/a/b", "//server/a/b/././.");
    normalizeP__AEqual__NES("/server/a", "//server/./a/");
    normalizeP__AEqual__NES("/server", "//server/./");
    normalizeP__AEqual__NES("/server", "//server/.");

    normalizeP__AEqual__NES("/a/b/c.txt", "/./a/b/c.txt");
    normalizeP__AEqual__NES("/127.0.0.256/a/b/c.txt", "//./127.0.0.256/./a/b/c.txt");
}

void testNormalizePath__DoubleDots() {
    logVerbose(LOG_TAG, "testNormalizePath__DoubleDots()");

    normalizeP__ANull__KES("..");
    normalizeP__ANull__KES("../");
    normalizeP__AEqual__KES("/", "/..");
    normalizeP__AEqual__KES("/", "/../");
    normalizeP__AEqual__KES("/", "/../../");

    normalizeP__AEqual__KES("/a", "/../a");
    normalizeP__AEqual__KES("/a", "/../a/.");
    normalizeP__ANull__KES("../");
    normalizeP__ANull__KES("../a");
    normalizeP__ANull__KES("../a/.");

    normalizeP__ANull__KES("./../a");
    normalizeP__ANull__KES("././../a");
    normalizeP__ANull__KES("././.././a");

    normalizeP__ANull__KES("a/../../b");
    normalizeP__AEqual__KES("b", "a/./../b");
    normalizeP__AEqual__KES("b", "a/./.././b");

    normalizeP__AEqual__KES("a/c", "a/b/../c");
    normalizeP__AEqual__KES("c", "a/b/../../c");
    normalizeP__AEqual__KES("c/", "a/b/../../c/");
    normalizeP__ANull__KES("a/b/../../../c");
    normalizeP__AEqual__KES("a", "a/b/..");
    normalizeP__AEqual__KES("a/", "a/b/../");
    normalizeP__ANull__KES("a/b/../..");
    normalizeP__ANull__KES("a/b/../../");
    normalizeP__ANull__KES("a/b/../../..");
    normalizeP__AEqual__KES("a/d", "a/b/../c/../d");
    normalizeP__AEqual__KES("a/d/", "a/b/../c/../d/");
    normalizeP__AEqual__KES("/a/c", "/a/b/../c");
    normalizeP__AEqual__KES("/c", "/a/b/../../c");
    normalizeP__AEqual__KES("/c", "/a/b/../../../c");
    normalizeP__AEqual__KES("/a", "/a/b/..");
    normalizeP__AEqual__KES("/", "/a/b/../..");
    normalizeP__AEqual__KES("/", "/a/b/../../..");
    normalizeP__AEqual__KES("/a/d", "/a/b/../c/../d");
    normalizeP__AEqual__KES("/a", "/../a");

    normalizeP__ANull__KES("~/..");
    normalizeP__ANull__KES("~/../");
    normalizeP__ANull__KES("~/../a");
    normalizeP__ANull__KES("~/../a/.");
    normalizeP__AEqual__KES("~/a/c", "~/a/b/../c");
    normalizeP__AEqual__KES("~/c", "~/a/b/../../c");
    normalizeP__ANull__KES("~/a/b/../../../c");
    normalizeP__AEqual__KES("~/a", "~/a/b/..");
    normalizeP__AEqual__KES("~/", "~/a/b/../../");
    normalizeP__AEqual__KES("~", "~/a/b/../..");
    normalizeP__ANull__KES("~/a/b/../../..");
    normalizeP__AEqual__KES("~/a/d", "~/a/b/../c/../d");
    normalizeP__ANull__KES("~/../a");
    normalizeP__ANull__KES("~/..");
    normalizeP__AEqual__KES("~user/a/c", "~user/a/b/../c");
    normalizeP__AEqual__KES("~user/c", "~user/a/b/../../c");
    normalizeP__ANull__KES("~user/a/b/../../../c");
    normalizeP__AEqual__KES("~user/a", "~user/a/b/..");
    normalizeP__AEqual__KES("~user", "~user/a/b/../..");
    normalizeP__ANull__KES("~user/a/b/../../..");
    normalizeP__AEqual__KES("~user/a/d", "~user/a/b/../c/../d");
    normalizeP__ANull__KES("~user/../a");
    normalizeP__ANull__KES("~user/..");

    normalizeP__AEqual__KES("C:/a/c", "C:/a/b/../c");
    normalizeP__AEqual__KES("C:/c", "C:/a/b/../../c");
    normalizeP__ANull__KES("C:/a/b/../../../../c");
    normalizeP__AEqual__KES("c", "C:/a/b/../../../c");
    normalizeP__AEqual__KES("C:/a", "C:/a/b/..");
    normalizeP__AEqual__KES("C:", "C:/a/b/../..");
    normalizeP__ANull__KES("C:/a/b/../../..");
    normalizeP__AEqual__KES("C:/a/d", "C:/a/b/../c/../d");
    normalizeP__AEqual__KES("a", "C:/../a");
    normalizeP__ANull__KES("C:/..");
    normalizeP__AEqual__KES("C:a/c", "C:a/b/../c");
    normalizeP__AEqual__KES("c", "C:a/b/../../c");
    normalizeP__ANull__KES("C:a/b/../../../c");
    normalizeP__AEqual__KES("C:a", "C:a/b/..");
    normalizeP__ANull__KES("C:a/b/../..");
    normalizeP__ANull__KES("C:a/b/../../..");
    normalizeP__AEqual__KES("C:a/d", "C:a/b/../c/../d");
    normalizeP__AEqual__KES("C:../a", "C:../a");
    normalizeP__AEqual__KES("C:..", "C:..");

    normalizeP__AEqual__KES("/server/a/c", "//server/a/b/../c");
    normalizeP__AEqual__KES("/server/c", "//server/a/b/../../c");
    normalizeP__AEqual__KES("/c", "/server/a/b/../../../../c");
    normalizeP__AEqual__KES("/c", "//server/a/b/../../../c");
    normalizeP__AEqual__KES("/server/a", "//server/a/b/..");
    normalizeP__AEqual__KES("/server", "//server/a/b/../..");
    normalizeP__AEqual__KES("/", "//server/a/b/../../..");
    normalizeP__AEqual__KES("/server/a/d", "//server/a/b/../c/../d");
    normalizeP__AEqual__KES("/a", "//server/../a");
    normalizeP__AEqual__KES("/", "//server/../..");
    normalizeP__AEqual__KES("/", "//server/..");
    normalizeP__AEqual__KES("/b/c/g", "/a/../../../../b/c/d/e/f/../../../g");
    normalizeP__AEqual__KES("/g", "/a/../../../../b/c/d/e/f/../../../../../g");

    normalizeP__AEqual__KES("/a/b/c.txt", "//../a/b/c.txt");
    normalizeP__AEqual__KES("/127.0..1/a/b/c.txt", "//127.0..1/a/b/c.txt");
    normalizeP__AEqual__KES("/foo", "//../foo");
    normalizeP__AEqual__KES("/foo", "//../foo");



    normalizeP__ANull__NES("..");
    normalizeP__ANull__NES("../");
    normalizeP__AEqual__NES("/", "/..");
    normalizeP__AEqual__NES("/", "/../");
    normalizeP__AEqual__NES("/", "/../../");

    normalizeP__AEqual__NES("/a", "/../a/");
    normalizeP__AEqual__NES("/a", "/../a/./");
    normalizeP__ANull__NES("../a");
    normalizeP__ANull__NES("../a/.");
    normalizeP__ANull__NES("../a");

    normalizeP__ANull__NES("./../a");
    normalizeP__ANull__NES("././../a");
    normalizeP__ANull__NES("././.././a");

    normalizeP__ANull__NES("a/../../b");
    normalizeP__AEqual__NES("b", "a/./../b/");
    normalizeP__AEqual__NES("b", "a/./.././b/");

    normalizeP__AEqual__NES("a/c", "a/b/../c/");
    normalizeP__AEqual__NES("c", "a/b/../../c");
    normalizeP__AEqual__NES("c", "a/b/../../c/");
    normalizeP__ANull__NES("a/b/../../../c/");
    normalizeP__AEqual__NES("a", "a/b/..");
    normalizeP__AEqual__NES("a", "a/b/../");
    normalizeP__ANull__NES("a/b/../..");
    normalizeP__ANull__NES("a/b/../../");
    normalizeP__ANull__NES("a/b/../../../");
    normalizeP__AEqual__NES("a/d", "a/b/../c/../d");
    normalizeP__AEqual__NES("a/d", "a/b/../c/../d/");
    normalizeP__AEqual__NES("/a/c", "/a/b/../c/");
    normalizeP__AEqual__NES("/c", "/a/b/../../c/");
    normalizeP__AEqual__NES("/c", "/a/b/../../../c/");
    normalizeP__AEqual__NES("/a", "/a/b/../");
    normalizeP__AEqual__NES("/", "/a/b/../../");
    normalizeP__AEqual__NES("/", "/a/b/../../../");
    normalizeP__AEqual__NES("/a/d", "/a/b/../c/../d/");
    normalizeP__AEqual__NES("/a", "/../a/");

    normalizeP__ANull__NES("~/..");
    normalizeP__ANull__NES("~/../");
    normalizeP__ANull__NES("~/../a");
    normalizeP__ANull__NES("~/../a/.");
    normalizeP__AEqual__NES("~/a/c", "~/a/b/../c/");
    normalizeP__AEqual__NES("~/c", "~/a/b/../../c/");
    normalizeP__ANull__NES("~/a/b/../../../c/");
    normalizeP__AEqual__NES("~/a", "~/a/b/../");
    normalizeP__AEqual__NES("~", "~/a/b/../../");
    normalizeP__AEqual__NES("~", "~/a/b/../..");
    normalizeP__ANull__NES("~/a/b/../../../");
    normalizeP__AEqual__NES("~/a/d", "~/a/b/../c/../d/");
    normalizeP__ANull__NES("~/../a/");
    normalizeP__ANull__NES("~/..");
    normalizeP__ANull__NES("~/../");
    normalizeP__AEqual__NES("~user/a/c", "~user/a/b/../c/");
    normalizeP__AEqual__NES("~user/c", "~user/a/b/../../c/");
    normalizeP__ANull__NES("~user/a/b/../../../c/");
    normalizeP__AEqual__NES("~user/a", "~user/a/b/../");
    normalizeP__AEqual__NES("~user", "~user/a/b/../../");
    normalizeP__ANull__NES("~user/a/b/../../../");
    normalizeP__AEqual__NES("~user/a/d", "~user/a/b/../c/../d/");
    normalizeP__ANull__NES("~user/../a/");
    normalizeP__ANull__NES("~user/../");

    normalizeP__AEqual__NES("C:/a/c", "C:/a/b/../c/");
    normalizeP__AEqual__NES("C:/c", "C:/a/b/../../c/");
    normalizeP__AEqual__NES("c", "C:/a/b/../../../c/");
    normalizeP__AEqual__NES("C:/a", "C:/a/b/../");
    normalizeP__AEqual__NES("C:", "C:/a/b/../../");
    normalizeP__ANull__NES("C:/a/b/../../../");
    normalizeP__AEqual__NES("C:/a/d", "C:/a/b/../c/../d/");
    normalizeP__AEqual__NES("a", "C:/../a/");
    normalizeP__ANull__NES("C:/../");
    normalizeP__AEqual__NES("C:a/c", "C:a/b/../c/");
    normalizeP__AEqual__NES("c", "C:a/b/../../c/");
    normalizeP__ANull__NES("C:a/b/../../../c/");
    normalizeP__AEqual__NES("C:a", "C:a/b/../");
    normalizeP__ANull__NES("C:a/b/../../");
    normalizeP__ANull__NES("C:a/b/../../../");
    normalizeP__AEqual__NES("C:a/d", "C:a/b/../c/../d/");
    normalizeP__AEqual__NES("C:../a", "C:../a/");
    normalizeP__AEqual__NES("C:..", "C:../");

    normalizeP__AEqual__NES("/server/a/c", "//server/a/b/../c/");
    normalizeP__AEqual__NES("/server/c", "//server/a/b/../../c/");
    normalizeP__AEqual__NES("/c", "//server/a/b/../../../../c/");
    normalizeP__AEqual__NES("/c", "//server/a/b/../../../c/");
    normalizeP__AEqual__NES("/server/a", "//server/a/b/../");
    normalizeP__AEqual__NES("/server", "//server/a/b/../../");
    normalizeP__AEqual__NES("/", "//server/a/b/../../../");
    normalizeP__AEqual__NES("/server/a/d", "//server/a/b/../c/../d/");
    normalizeP__AEqual__NES("/a", "//server/../a");
    normalizeP__AEqual__NES("/", "//server/../../");
    normalizeP__AEqual__NES("/", "//server/../");
}

void testNormalizePath__FromMethodDocs() {
    logVerbose(LOG_TAG, "testNormalizePath__FromMethodDocs()");

    normalizeP__ANull(NULL, false, true);
    normalizeP__ANull__NES("");
    normalizeP__AEqual__NES("/", "/");
    normalizeP__AEqual__NES("/", "//");

    normalizeP__AEqual__NES(":", ":");
    normalizeP__AEqual__NES("1:/a/b/c.txt", "1:/a/b/c.txt");
    normalizeP__AEqual__NES("/a/b/c/d", "///a//b///c/d///");
    normalizeP__AEqual__NES("/foo", "/foo//");
    normalizeP__AEqual__NES("/foo", "/foo/./");
    normalizeP__AEqual__NES("/foo/bar", "//foo//./bar");
    normalizeP__AEqual__NES("~", "~");
    normalizeP__AEqual__NES("~", "~/");
    normalizeP__AEqual__NES("~user", "~user/");
    normalizeP__AEqual__NES("~user", "~user");
    normalizeP__AEqual__NES("~user/a", "~user/a");

    normalizeP__ANull__NES(".");
    normalizeP__ANull__NES("./");
    normalizeP__AEqual__NES("/", "/.");
    normalizeP__AEqual__NES("/", "/./");
    normalizeP__ANull__NES("././");
    normalizeP__AEqual__NES("/", "/./.");
    normalizeP__AEqual__NES("/a", "/././a");
    normalizeP__AEqual__NES("/b", "/./b/./");
    normalizeP__AEqual__NES("c", "././c/./");

    normalizeP__ANull__NES("..");
    normalizeP__ANull__NES("../");
    normalizeP__AEqual__NES("/", "/..");
    normalizeP__AEqual__NES("/", "/../");
    normalizeP__AEqual__NES("/a", "/../a");
    normalizeP__ANull__NES("../a/.");
    normalizeP__ANull__NES("../a");
    normalizeP__AEqual__NES("/a", "/a/b/..");
    normalizeP__AEqual__NES("/", "/a/b/../..");
    normalizeP__AEqual__NES("/", "/a/b/../../..");
    normalizeP__AEqual__NES("/c", "/a/b/../../../c");
    normalizeP__ANull__NES("./../a");
    normalizeP__ANull__NES("././../a");
    normalizeP__ANull__NES("././.././a");
    normalizeP__ANull__NES("a/../../b");
    normalizeP__AEqual__NES("b", "a/./../b");
    normalizeP__AEqual__NES("b", "a/./.././b");
    normalizeP__ANull__NES("foo/../../bar");
    normalizeP__AEqual__NES("/bar", "/foo/../bar");
    normalizeP__AEqual__NES("/bar", "/foo/../bar/");
    normalizeP__AEqual__NES("/baz", "/foo/../bar/../baz");
    normalizeP__AEqual__NES("foo", "foo/bar/..");
    normalizeP__AEqual__NES("bar", "foo/../bar");
    normalizeP__ANull__NES("~/..");
    normalizeP__ANull__NES("~/../");
    normalizeP__ANull__NES("~/../a/.");
    normalizeP__ANull__NES("~/..");
    normalizeP__AEqual__NES("~/bar", "~/foo/../bar/");
    normalizeP__AEqual__NES("C:/bar", "C:/foo/../bar");
    normalizeP__AEqual__NES("/server/bar", "//server/foo/../bar");
    normalizeP__AEqual__NES("/bar", "//server/../bar");
}

void testNormalizePath__BinaryPaths() {
    logVerbose(LOG_TAG, "testNormalizePath__BinaryPaths()");

    normalizeP__AEqual__NES("binary", "binary");
    normalizeP__AEqual__NES("binary", "binary/");
    normalizeP__AEqual__NES("/binary", "//binary//");
    normalizeP__AEqual__NES("/binary", "/../binary");
    normalizeP__AEqual__NES("/binary", "/../../binary");
    normalizeP__ANull__NES("../binary");

    normalizeP__AEqual__NES("path/to/binary", "path/to/binary");
    normalizeP__AEqual__NES("path/to/binary", "path/to/binary/");
    normalizeP__AEqual__NES("path/to/binary", "path//to//binary");

    normalizeP__AEqual__NES("path/path/to/binary", "path/./path/to/binary");
    normalizeP__AEqual__NES("/path/to/binary", "/././path/to/binary");
    normalizeP__AEqual__NES("path/to/binary", "././path/to/binary");

    normalizeP__AEqual__NES("path/to/binary", "path/../path/to/binary");
    normalizeP__AEqual__NES("/path/to/binary", "/path/../../path/to/binary");
    normalizeP__ANull__NES("path/../../path/to/binary");

    normalizeP__AEqual__NES("path/to/binary", "path/.././path/to/binary");
    normalizeP__AEqual__NES("path/to/binary", "path/./../path/to/binary");
    normalizeP__AEqual__NES("/path/to/binary", "/path/../../path/to/binary");
    normalizeP__AEqual__NES("/path/to/binary", "/path/./../../path/to/binary");
    normalizeP__AEqual__NES("/path/to/binary", "/path/../.././path/to/binary");
    normalizeP__AEqual__NES("/path/to/binary", "/path/./../.././path/to/binary");
    normalizeP__ANull__NES("path/./../../path/to/binary");
    normalizeP__ANull__NES("path/./../.././path/to/binary");

    normalizeP__AEqual__NES("/usr/bin/sh", "/../../../../../../../../../../../../usr/./bin/../bin/sh");
    normalizeP__ANull__NES("../../../../../../../../../../../../usr/./bin/../bin/sh");

    normalizeP__AEqual__NES(TERMUX_BIN_DIR "/sh", TERMUX_BIN_DIR "/sh");
    normalizeP__AEqual__NES(TERMUX_BIN_DIR "/sh", TERMUX_BIN_DIR "/../bin/sh");
}





void testGetTermuxRootDirFromEnv__Basic();

void runGetTermuxRootDirFromEnvTests() {
    logDebug(LOG_TAG, "runGetTermuxRootDirFromEnvTests()");

    testGetTermuxRootDirFromEnv__Basic();

    int__AEqual(0, errno);
}

void testGetTermuxRootDirFromEnv__Basic() {
    logVerbose(LOG_TAG, "testGetTermuxRootDirFromEnv__Basic()");

    size_t env_var_size = strlen(ENV__TERMUX__ROOTFS) + 1;

    char buffer[TERMUX__ROOTFS_DIR_MAX_LEN];
    size_t len = sizeof(buffer);

    int result;

    putenv(ENV__TERMUX__ROOTFS "=" TERMUX__ROOTFS);
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 0);
    string__AEqual(TERMUX__ROOTFS, buffer);

    putenv(ENV__TERMUX__ROOTFS "=/");
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 0);
    string__AEqual("/", buffer);

    putenv(ENV__TERMUX__ROOTFS "=/a");
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 0);
    string__AEqual("/a", buffer);

    putenv(ENV__TERMUX__ROOTFS "=a");
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 1);

    putenv(ENV__TERMUX__ROOTFS "=a/");
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 1);

    putenv(ENV__TERMUX__ROOTFS "=a" TERMUX__ROOTFS);
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 1);

    unsetenv(ENV__TERMUX__ROOTFS);
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 1);

    state__ATrue(PATH_MAX >= TERMUX__ROOTFS_DIR_MAX_LEN);

    char rootfs_dir1[env_var_size + PATH_MAX];
    snprintf(rootfs_dir1, sizeof(rootfs_dir1), "%s=/%0*d", ENV__TERMUX__ROOTFS, TERMUX__ROOTFS_DIR_MAX_LEN - 1, 0);

    putenv(rootfs_dir1);
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 1);

    char rootfs_dir2[env_var_size + PATH_MAX];
    size_t rootfs_dir2_length = snprintf(rootfs_dir2, sizeof(rootfs_dir2), "%s=/%0*d", ENV__TERMUX__ROOTFS, TERMUX__ROOTFS_DIR_MAX_LEN - 2, 0);

    putenv(rootfs_dir2);
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer, len);
    state__ATrue(result == 0);
    state__ATrue(rootfs_dir2_length - env_var_size == strlen(buffer));

    char buffer1[TERMUX__ROOTFS_DIR_MAX_LEN - 1];
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer1, sizeof(buffer1));
    state__ATrue(result == -1);
    errno = 0;

    char buffer2[TERMUX__ROOTFS_DIR_MAX_LEN - 2];
    result = get_termux_rootfs_dir_from_env(LOG_TAG, buffer2, sizeof(buffer2));
    state__ATrue(result == -1);
    errno = 0;
}





void testTermuxPrefixPath__Basic();

void runTermuxPrefixPathTests() {
    logDebug(LOG_TAG, "runTermuxPrefixPathTests()");

    testTermuxPrefixPath__Basic();

    int__AEqual(0, errno);
}



#define tpp__AEqual(expected, termux_rootfs_dir, executable_path, buffer, buffer_len)              \
    if (1) {                                                                                       \
    assert_string_equals_with_error(expected, termux_prefix_path(LOG_TAG, termux_rootfs_dir, executable_path, buffer, buffer_len), \
        LOG_TAG, "%d: termux_prefix_path('%s', '%s')", __LINE__, termux_rootfs_dir , executable_path); \
    errno = 0;                                                                                     \
    } else ((void)0)



void testTermuxPrefixPath__Basic() {
    logVerbose(LOG_TAG, "testTermuxPrefixPath__Basic()");

    char buf[PATH_MAX];

    bool termux_rootfs_is_rootfs = strcmp(TERMUX__ROOTFS, "/") == 0;

    if (!termux_rootfs_is_rootfs) {
        tpp__AEqual(TERMUX_BIN_DIR, TERMUX__ROOTFS, "/bin", buf, PATH_MAX);
        tpp__AEqual(TERMUX_BIN_DIR "/", TERMUX__ROOTFS, "/bin/", buf, PATH_MAX);
        tpp__AEqual(TERMUX_BIN_DIR, TERMUX__ROOTFS, "/usr/bin", buf, PATH_MAX);
        tpp__AEqual(TERMUX_BIN_DIR "/", TERMUX__ROOTFS, "/usr/bin/", buf, PATH_MAX);
        tpp__AEqual(TERMUX_BIN_DIR "/sh", TERMUX__ROOTFS, "/bin/sh", buf, PATH_MAX);
        tpp__AEqual(TERMUX_BIN_DIR "/sh", TERMUX__ROOTFS, "/usr/bin/sh", buf, PATH_MAX);
        tpp__AEqual("/system/bin/sh", TERMUX__ROOTFS, "/system/bin/sh", buf, PATH_MAX);
        tpp__AEqual("/system/bin/tool", TERMUX__ROOTFS, "/system/bin/tool", buf, PATH_MAX);
        tpp__AEqual(TERMUX_BIN_DIR "/sh", TERMUX__ROOTFS, TERMUX_BIN_DIR "/sh", buf, PATH_MAX);

        tpp__AEqual("./ab/sh", TERMUX__ROOTFS, "./ab/sh", buf, PATH_MAX);
    } else {
        tpp__AEqual("/bin", TERMUX__ROOTFS, "/bin", buf, PATH_MAX);
        tpp__AEqual("/bin/", TERMUX__ROOTFS, "/bin/", buf, PATH_MAX);
        tpp__AEqual("/usr/bin", TERMUX__ROOTFS, "/usr/bin", buf, PATH_MAX);
        tpp__AEqual("/usr/bin/", TERMUX__ROOTFS, "/usr/bin/", buf, PATH_MAX);
        tpp__AEqual("/bin/sh", TERMUX__ROOTFS, "/bin/sh", buf, PATH_MAX);
        tpp__AEqual("/usr/bin/sh", TERMUX__ROOTFS, "/usr/bin/sh", buf, PATH_MAX);
        tpp__AEqual("/system/bin/sh", TERMUX__ROOTFS, "/system/bin/sh", buf, PATH_MAX);
        tpp__AEqual("/system/bin/tool", TERMUX__ROOTFS, "/system/bin/tool", buf, PATH_MAX);
        tpp__AEqual(TERMUX_BIN_DIR "/sh", TERMUX__ROOTFS, TERMUX_BIN_DIR "/sh", buf, PATH_MAX);

        tpp__AEqual("./ab/sh", TERMUX__ROOTFS, "./ab/sh", buf, PATH_MAX);
    }
}





void testIsPathUnderTermuxRootfsDir__Basic();

void runIsPathUnderTermuxRootfsDirTests() {
    logDebug(LOG_TAG, "runIsPathUnderTermuxRootfsDirTests()");

    testIsPathUnderTermuxRootfsDir__Basic();

    int__AEqual(0, errno);
}



#define iputrd__ABool(expected, path, termux_rootfs_dir)                                           \
    if (1) {                                                                                       \
    assert_int_with_error(expected ? 0 : 1, is_path_under_termux_rootfs_dir(LOG_TAG, path, termux_rootfs_dir), \
        LOG_TAG, "%d: is_path_under_termux_rootfs_dir('%s', '%s')", __LINE__, path, termux_rootfs_dir); \
    errno = 0;                                                                                     \
    } else ((void)0)

#define iputrd__AInt(expected, path, termux_rootfs_dir)                                            \
    if (1) {                                                                                       \
    assert_int_with_error(expected, is_path_under_termux_rootfs_dir(LOG_TAG, path, termux_rootfs_dir), \
        LOG_TAG, "%d: is_path_under_termux_rootfs_dir('%s', '%s')", __LINE__, path, termux_rootfs_dir); \
    errno = 0;                                                                                     \
    } else ((void)0)



void testIsPathUnderTermuxRootfsDir__Basic() {
    logVerbose(LOG_TAG, "testIsPathUnderTermuxRootfsDir__Basic()");

    bool termux_rootfs_is_rootfs = strcmp(TERMUX__ROOTFS, "/") == 0;
    bool termux_rootfs_is_system = strcmp(TERMUX__ROOTFS, "/system") == 0;

    state__ATrue(strlen(TERMUX__ROOTFS) < PATH_MAX);

    iputrd__ABool(false, NULL, TERMUX__ROOTFS);
    iputrd__ABool(false, "", TERMUX__ROOTFS);
    iputrd__ABool(false, "/", TERMUX__ROOTFS);

    iputrd__ABool(termux_rootfs_is_rootfs, "/a", TERMUX__ROOTFS);
    iputrd__ABool(termux_rootfs_is_rootfs, "/bin/sh", TERMUX__ROOTFS);
    iputrd__ABool(termux_rootfs_is_rootfs, "/usr/bin/sh", TERMUX__ROOTFS);
    iputrd__ABool(termux_rootfs_is_rootfs || termux_rootfs_is_system, "/system/bin/sh", TERMUX__ROOTFS);

    iputrd__ABool(true, TERMUX_BIN_DIR "/sh", TERMUX__ROOTFS);
    iputrd__ABool(termux_rootfs_is_rootfs, "/" TERMUX_BIN_DIR "/sh", TERMUX__ROOTFS);
    iputrd__ABool(termux_rootfs_is_rootfs, "/./" TERMUX_BIN_DIR "/sh", TERMUX__ROOTFS);
    iputrd__ABool(termux_rootfs_is_rootfs, "/../" TERMUX_BIN_DIR "/sh", TERMUX__ROOTFS);


    if (access(TERMUX_BIN_DIR "/sh", X_OK) == 0) {
        int fd = open(TERMUX_BIN_DIR "/sh", 0);
        state__ATrue(fd != -1);

        char real_path[PATH_MAX];
        ssize_t length = readlink(TERMUX_BIN_DIR "/sh", real_path, sizeof(real_path) - 1);
        state__ATrue(length >= 0);
        real_path[length] = '\0';

        // If not running on device, then `sh` would be a symlink to `/bin/sh` to `/usr/bin/dash`.
        bool is_not_on_device = string_starts_with(real_path, "/usr/bin/") || string_starts_with(real_path, "/bin/");

        char proc_fd_path[40];
        snprintf(proc_fd_path, sizeof(proc_fd_path), "/proc/self/fd/%d", fd);
        iputrd__AInt(is_not_on_device ? 1 : 0, proc_fd_path, TERMUX__ROOTFS);
        close(fd);
    } else {
        errno = 0;
    }


    if (access("/system/bin/sh", X_OK) == 0) {
        int fd = open("/system/bin/sh", 0);
        state__ATrue(fd != -1);
        char proc_fd_path[40];
        snprintf(proc_fd_path, sizeof(proc_fd_path), "/proc/self/fd/%d", fd);
        iputrd__ABool(termux_rootfs_is_rootfs || termux_rootfs_is_system, proc_fd_path, TERMUX__ROOTFS);
        close(fd);
    } else {
        errno = 0;
    }


    if (access(TERMUX_BIN_DIR, X_OK) == 0) {
        // S_ISREG should fail in get_fd_realpath()
        int fd = open(TERMUX_BIN_DIR, 0);
        state__ATrue(fd != -1);
        char proc_fd_path[40];
        snprintf(proc_fd_path, sizeof(proc_fd_path), "/proc/self/fd/%d", fd);
        iputrd__AInt(-1, proc_fd_path, TERMUX__ROOTFS);
        close(fd);
    } else {
        errno = 0;
    }
}





void testInspectFileHeader__Basic();

void runInspectFileHeaderTests() {
    logDebug(LOG_TAG, "runInspectFileHeaderTests()");

    testInspectFileHeader__Basic();

    int__AEqual(0, errno);
}

void testInspectFileHeader__Basic() {
    logVerbose(LOG_TAG, "testInspectFileHeader__Basic()");

    char header[FILE_HEADER__BUFFER_LEN];
    size_t hsize = sizeof(header);

    struct file_header_info info = {.interpreter_arg = NULL};

    snprintf(header, hsize, "#!/bin/sh\n");
    inspect_file_header(TERMUX__ROOTFS, header, sizeof(header) , &info);
    state__ATrue(!info.is_elf);
    state__ATrue(!info.is_non_native_elf);
    string__AEqual(TERMUX_BIN_DIR "/sh", info.interpreter_path);
    state__ATrue(info.interpreter_arg == NULL);

    snprintf(header, hsize, "#!/bin/sh -x\n");
    inspect_file_header(TERMUX__ROOTFS, header, sizeof(header) , &info);
    state__ATrue(!info.is_elf);
    state__ATrue(!info.is_non_native_elf);
    string__AEqual(TERMUX_BIN_DIR "/sh", info.interpreter_path);
    string__AEqual("-x", info.interpreter_arg);

    snprintf(header, hsize, "#! /bin/sh -x\n");
    inspect_file_header(TERMUX__ROOTFS, header, sizeof(header) , &info);
    state__ATrue(!info.is_elf);
    state__ATrue(!info.is_non_native_elf);
    string__AEqual(TERMUX_BIN_DIR "/sh", info.interpreter_path);
    string__AEqual("-x", info.interpreter_arg);

    snprintf(header, hsize, "#!/bin/sh -x \n");
    inspect_file_header(TERMUX__ROOTFS, header, sizeof(header) , &info);
    state__ATrue(!info.is_elf);
    state__ATrue(!info.is_non_native_elf);
    string__AEqual(TERMUX_BIN_DIR "/sh", info.interpreter_path);
    string__AEqual("-x", info.interpreter_arg);

    snprintf(header, hsize, "#!/bin/sh -x \n");
    inspect_file_header(TERMUX__ROOTFS, header, sizeof(header) , &info);
    state__ATrue(!info.is_elf);
    state__ATrue(!info.is_non_native_elf);
    string__AEqual(TERMUX_BIN_DIR "/sh", info.interpreter_path);
    string__AEqual("-x", info.interpreter_arg);

    info.interpreter_path = NULL;
    info.interpreter_arg = NULL;
    // An ELF header for a 32-bit file.
    // See https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#File_header
    snprintf(header, hsize, "\177ELF");
    // Native instruction set:
    header[0x12] = EM_NATIVE;
    header[0x13] = 0;
    inspect_file_header(TERMUX__ROOTFS, header, sizeof(header) , &info);
    state__ATrue(info.is_elf);
    state__ATrue(!info.is_non_native_elf);
    state__ATrue(info.interpreter_path == NULL);
    state__ATrue(info.interpreter_arg == NULL);

    info.interpreter_path = NULL;
    info.interpreter_arg = NULL;
    // An ELF header for a 64-bit file.
    // See https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#File_header
    snprintf(header, hsize, "\177ELF");
    // 'Fujitsu MMA Multimedia Accelerator' instruction set - likely non-native.
    header[0x12] = 0x36;
    header[0x13] = 0;
    inspect_file_header(TERMUX__ROOTFS, header, sizeof(header) , &info);
    state__ATrue(info.is_elf);
    state__ATrue(info.is_non_native_elf);
    state__ATrue(info.interpreter_path == NULL);
    state__ATrue(info.interpreter_arg == NULL);
}





void testModifyExecEnv__unsetLDVars();
void testModifyExecEnv__setProcSelfExe();

void runModifyExecEnvTests() {
    logDebug(LOG_TAG, "runModifyExecEnvTests()");

    testModifyExecEnv__unsetLDVars();
    testModifyExecEnv__setProcSelfExe();

    int__AEqual(0, errno);
}

void testModifyExecEnv__unsetLDVars() {
    logVerbose(LOG_TAG, "testModifyExecEnv__unsetLDVars()");

    {
    char *test_env[] = {"MY_ENV=1", NULL};
    char **allocated_envp;
    modify_exec_env(test_env, &allocated_envp, NULL, true);
    state__ATrue(allocated_envp != NULL);
    string__AEqual(allocated_envp[0], "MY_ENV=1");
    state__ATrue(allocated_envp[1] == NULL);
    free(allocated_envp);
    }

    {
    char *test_env[] = {"MY_ENV=1", ENV_PREFIX__LD_PRELOAD "a", NULL};
    char **allocated_envp;
    modify_exec_env(test_env, &allocated_envp, NULL, true);
    state__ATrue(allocated_envp != NULL);
    string__AEqual(allocated_envp[0], "MY_ENV=1");
    state__ATrue(allocated_envp[1] == NULL);
    free(allocated_envp);
    }

    {
    char *test_env[] = {"MY_ENV=1", ENV_PREFIX__LD_PRELOAD "a", "A=B", ENV_PREFIX__LD_LIBRARY_PATH "B", "B=C", NULL};
    char **allocated_envp;
    modify_exec_env(test_env, &allocated_envp, NULL, true);
    state__ATrue(allocated_envp != NULL);
    string__AEqual(allocated_envp[0], "MY_ENV=1");
    string__AEqual(allocated_envp[1], "A=B");
    string__AEqual(allocated_envp[2], "B=C");
    state__ATrue(allocated_envp[3] == NULL);
    free(allocated_envp);
    }
}

void testModifyExecEnv__setProcSelfExe() {
    logVerbose(LOG_TAG, "testModifyExecEnv__setProcSelfExe()");

    {
    char *termux_proc_self_exe = NULL;
    state__ATrue(asprintf(&termux_proc_self_exe, "%s%s", ENV_PREFIX__TERMUX_EXEC__PROC_SELF_EXE, TERMUX_BIN_DIR "/bash") != -1);

    char *test_env[] = {"MY_ENV=1", NULL};
    char **allocated_envp;
    modify_exec_env(test_env, &allocated_envp, &termux_proc_self_exe, false);
    state__ATrue(allocated_envp != NULL);
    string__AEqual(allocated_envp[0], "MY_ENV=1");
    string__AEqual(allocated_envp[1], ENV__TERMUX_EXEC__PROC_SELF_EXE "=" TERMUX_BIN_DIR "/bash");
    free(termux_proc_self_exe);
    free(allocated_envp);
    }

    {
    char *termux_proc_self_exe = NULL;
    state__ATrue(asprintf(&termux_proc_self_exe, "%s%s", ENV_PREFIX__TERMUX_EXEC__PROC_SELF_EXE, TERMUX_BIN_DIR "/bash") != -1);

    char *test_env[] = {"MY_ENV=1", ENV__TERMUX_EXEC__PROC_SELF_EXE "=" TERMUX_BIN_DIR "/python", NULL};
    char **allocated_envp;
    modify_exec_env(test_env, &allocated_envp, &termux_proc_self_exe, false);
    state__ATrue(allocated_envp != NULL);
    string__AEqual(allocated_envp[0], "MY_ENV=1");
    string__AEqual(allocated_envp[1], ENV__TERMUX_EXEC__PROC_SELF_EXE "=" TERMUX_BIN_DIR "/bash");
    free(termux_proc_self_exe);
    free(allocated_envp);
    }

    {
    char *test_env[] = {"MY_ENV=1", NULL};
    char **allocated_envp;
    modify_exec_env(test_env, &allocated_envp, NULL, false);
    state__ATrue(allocated_envp != NULL);
    string__AEqual(allocated_envp[0], "MY_ENV=1");
    state__ATrue(allocated_envp[1] == NULL);
    free(allocated_envp);
    }

    {
    char *test_env[] = {"MY_ENV=1", ENV__TERMUX_EXEC__PROC_SELF_EXE "=" TERMUX_BIN_DIR "/python", NULL};
    char **allocated_envp;
    modify_exec_env(test_env, &allocated_envp, NULL, false);
    state__ATrue(allocated_envp != NULL);
    string__AEqual(allocated_envp[0], "MY_ENV=1");
    state__ATrue(allocated_envp[1] == NULL);
    free(allocated_envp);
    }
}





int main() {
    init();

    logInfo(LOG_TAG, "Start \"unit\" tests");

    runStringStartsWithTests();
    runStringEndsWithTests();
    runAbsolutizePathTests();
    runNormalizePathTests();
    runGetTermuxRootDirFromEnvTests();
    runTermuxPrefixPathTests();
    runIsPathUnderTermuxRootfsDirTests();
    runInspectFileHeaderTests();
    runModifyExecEnvTests();

    logInfo(LOG_TAG, "End \"unit\" tests");

    return 0;
}



static void init() {
    errno = 0;

    initLogger();
}

static void initLogger() {
    setDefaultLogTagAndPrefix(TERMUX__NAME);
    setCurrentLogLevel(get_termux_exec_tests_log_level());
    setFormattedLogging(false);
}

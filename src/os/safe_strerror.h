/*
 * Copyright 2011 The Chromium Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 * - https://github.com/chromium/chromium/blob/e4622aaeccea84652488d1822c28c78b7115684f/LICENSE
 * - https://github.com/chromium/chromium/blob/e4622aaeccea84652488d1822c28c78b7115684f/base/posix/safe_strerror.h
 */

#ifndef SAFE_STRERROR_H
#define SAFE_STRERROR_H

#include <stddef.h>

/**
 * Buffer size for a `strerror()` description.
 */
#define STRERROR_BUFFER_SIZE 256

/**
 * Get the `errno` description with `strerror()` call in the `buf` passed.
 *
 * This method declares safe, portable alternative to the POSIX `strerror()` function.
 * The `strerror()` is inherently unsafe in multi-threaded apps and should never be used.
 * Doing so can cause crashes. Additionally, the thread-safe alternative `strerror_r()` varies in
 * semantics across platforms. Use these functions instead.
 *
 * This method is a thread-safe `strerror()` function with dependable semantics that never fails.
 * It will write the string form of error `errnoCode` to buffer `buf` of length `len`.
 * If there is an error calling the OS's `strerror_r()` function then a message to that effect will
 * be printed into `buf`, truncating if necessary. The final result is always null-terminated.
 * The value of `errno` is never changed.
 *
 * Use this instead of `strerror_r()`.
 *
 * - https://man7.org/linux/man-pages/man3/errno.3.html
 * - https://man7.org/linux/man-pages/man3/strerror.3.html
 *
 * @param errnoCode The `errno` code to get description for.
 * @param buf The buffer to get description in.
 * @param len The buffer length.
 */
void safe_strerror_r(int errnoCode, char* buf, size_t len);

#endif // SAFE_STRERROR_H

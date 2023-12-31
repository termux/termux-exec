#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "../data/data_utils.h"
#include "file_utils.h"

/*
 * Copyright (c) 1994, 2010, Oracle and/or its affiliates.
 * Copyright (c) 2023 stargateoss.
 *
 * All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

/**
 * Pathname canonicalization for Unix file systems.
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:libcore/ojluni/src/main/native/canonicalize_md.c
 */

/**
 * Check the given name sequence to see if it can be further collapsed.
 * Return zero if not, otherwise return the number of names in the sequence.
 */
int __collapsible(char *names) {
    char *p = names;
    int dots = 0, n = 0;

    while (*p) {
        if ((p[0] == '.') && ((p[1] == '\0')
                                || (p[1] == '/')
                                || ((p[1] == '.') && ((p[2] == '\0')
                                                    || (p[2] == '/'))))) {
            dots = 1;
        }
        n++;
        while (*p) {
            if (*p == '/') {
                while (*p && *p == '/') {
                    p++;
                }
                break;
            }
            p++;
        }
    }
    return (dots ? n : 0);
}

/**
 * Split the names in the given name sequence, replacing slashes with nulls and filling in the given
 * index array.
 */
void __splitNames(char *names, char **ix) {
    char *p = names;
    int i = 0;

    while (*p) {
        ix[i++] = p++;
        while (*p) {
            if (*p == '/') {
                while (*p && *p == '/') {
                    *p++ = '\0';
                }
                break;
            }
            p++;
        }
    }
}

/**
 * Join the names in the given name sequence, ignoring names whose index entries have been cleared
 * and replacing nulls with slashes as needed.
 */
void __joinNames(char *names, int nc, char **ix) {
    int i;
    char *p;

    for (i = 0, p = names; i < nc; i++) {
        if (!ix[i]) continue;
        // Do not write to memory before start of path, like for case `././c/./`
        if (i > 0 && p > names) {
            p[-1] = '/';
        }
        if (p == ix[i]) {
            p += strlen(p) + 1;
        } else {
            char *q = ix[i];
            while ((*p++ = *q++));
        }
    }
    *p = '\0';
}


char* __normalizePath(char* path, bool keepEndSeparator, bool removeDoubleDot) {
    //fprintf(stderr, "path='%s'\n", path);
    if (path == NULL) { return NULL; }

    size_t pathLength = strlen(path);
    size_t originalPathLength = pathLength;
    if (pathLength < 1 || strcmp(path, ".") == 0 || strcmp(path, "..") == 0) { return NULL; }

    char originalPath[pathLength + 1];
    if (removeDoubleDot) {
        strcpy(originalPath, path);
    }

    bool endsWithSeparator = pathLength > 0 && path[pathLength - 1] == '/';

    // Replace consecutive duplicate separators `//` with a single separator `/`,
    // regardless of if single or double dot components exist.
    remove_dup_separator(path, true);
    if (strcmp(path, "/") == 0) { return path; }
    if (strcmp(path, "./") == 0 || strcmp(path, "../") == 0) { return NULL; }
    if (strcmp(path, "/..") == 0 || strcmp(path, "/../") == 0) {
        path[0] = '/'; path[1] = '\0';
        return path;
    }



    // Remove single `.` and double dot `..` path components if at least 2 components exist.
    bool isAbsolutePath = path[0] == '/';
    char* names = (isAbsolutePath) ? path + 1 : path; // Preserve first '/'

    int nc = __collapsible(names);

    if (nc >= 2) {
        int i, j;

        //char** ix = (char **) alloca(nc * sizeof(char *));
        size_t ix_size = (nc * sizeof(char *));
        void* result = malloc(ix_size);
        if (result == NULL) {
            logStrerrorDebug(LOG_TAG, "The malloc called failed for ix in 'normalizePath(%s)' with size '%zu'", path, ix_size);
            return NULL;
        }

        char **ix = (char **) result;

        __splitNames(names, ix);

        for (i = 0; i < nc; i++) {
            int dots = 0;

            // Find next occurrence of "." or "..".
            do {
                char *p = ix[i]; // NOLINT
                if (p != NULL && p[0] == '.') {
                    if (p[1] == '\0') {
                        dots = 1;
                        break;
                    }
                    if ((p[1] == '.') && (p[2] == '\0')) {
                        dots = 2;
                        break;
                    }
                }
                i++;
            } while (i < nc);
            if (i >= nc) break;

            // At this point i is the index of either a `.` or a `..`, so take the appropriate action and
            // then continue the outer loop.
            if (dots == 1) {
                // Remove this instance of `.`
                ix[i] = NULL;
            } else {
                if (removeDoubleDot) {
                    // Remove this instance of `..` and any preceding component if one exists and is possible.
                    for (j = i - 1; j >= 0; j--) {
                        if (ix[j]) break;
                    }

                    ix[i] = NULL;
                    if (j < 0) {
                         // If a preceding component is not found.
                        if (!isAbsolutePath) {
                            // If path is relative, then exit with error since we cannot remove
                            // unknown components.
                            free(ix);
                            return NULL;
                        } else {
                            // If path is absolute, then nothing to do, as paths above rootfs
                            // are to be reset to `/`.
                            continue;
                        }
                    } else {
                         // If a preceding component is found.
                        if (j == 0 && string_starts_with(ix[j], "~")) {
                            // If the first component equals `~` or `~user`, then exit with error
                            // since we cannot remove unknown components.
                            free(ix);
                            return NULL;
                        } else {
                            // Remove the component.
                            ix[j] = NULL;
                        }
                    }

                }
            }
            // i will be incremented at the top of the loop.
        }

        __joinNames(names, nc, ix);
        free(ix);

        if (keepEndSeparator && endsWithSeparator) {
            // Restore trailing `/`.
            pathLength = strlen(path);
            if (strcmp(path, "/") != 0 && pathLength > 0 && path[pathLength - 1] != '/' && (pathLength + 1) <= originalPathLength) {
                path[pathLength] = '/';
                path[pathLength + 1] = '\0';
            }
        }
    }

    if (*path == '\0') { return NULL; }
    //fprintf(stderr, "path1='%s'\n", path);



    // Handle cases where only one component exists and it is for a single dot `.` component as above
    // logic wouldn't run for it.

    // Remove ./ from start.
    if (string_starts_with(path, "./")) {
        if (strlen(path) == 2) return NULL;
        path = path + 2;
    }

    // Remove /. from end.
    if (string_ends_with(path, "/.")) {
        pathLength = strlen(path);
        if (pathLength == 2) {
            path[0] = '/'; path[1] = '\0';
            return path;
        }
        int trim = keepEndSeparator ? 1 : 2;
        path[pathLength - trim] = '\0';
    }

    if (strcmp(path, "/./") == 0) {
        path[0] = '/'; path[1] = '\0';
        return path;
    }

    // If path originally did not have separator at end or if user wants it removed.
    if (!endsWithSeparator || !keepEndSeparator) {
        // Remove trailing `/`.
        pathLength = strlen(path);
        if (strcmp(path, "/") != 0 && pathLength > 0 && path[pathLength - 1] == '/') {
            path[pathLength - 1] = '\0';
        }
    }

    return *path != '\0' ? path : NULL;
}

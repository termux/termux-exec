#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdbool.h>

/**
 * Absolutize a unix path with the current working directory (`cwd`) as
 * prefix for the path if its not absolute.
 */
char* absolutize_path(const char *path, char *absolute_path, int buffer_len);





/**
 * Normalize a unix path. This won't normalize a windows path with backslashes and other restrictions.
 *
 * - Replaces consecutive duplicate path separators `//` with a single path separator `/`.
 * - Removes trailing path separator `/` if `keepEndSeparator` is not `true`.
 * - Replaces `/./` with `/`.
 * - Removes `./` from start.
 * - Removes `/.` from end.
 * - Optionally removes `/../` and `/..` along with the parent directories equal to count of `..`.
 *
 * - If path is `null`, empty or equals `.`, then `null` will be returned.
 * - If path is empty after removals, then `null` will be returned.
 * - If double dots `..` need to be removed and no parent directory path components exist in the path
 *   that can be removed, then
 *   - If path is an absolute path, then path above rootfs will reset to `/` and any remaining
 *     path will be kept as well, like `/a/b/../../../c` will be resolved to `/c`.
 *     This behaviour is same as {@link #canonicalizePath(char*, char*, size_t)} and `realpath`.
 *     Note that [`org.apache.commons.io.FilenameUtils#normalize(String)`]
 *     will return `null`, but this implementation will not since it makes more sense to reset
 *     path to `/` for unix filesystems. See also `CVE-2021-29425` for a path traversal
 *     vulnerability for `//../foo` in `commons.io` that was fixed in `2.7` (currently using `2.5`).
 *     - https://nvd.nist.gov/vuln/detail/cve-2021-29425
 *     - https://issues.apache.org/jira/browse/IO-556
 *     - https://issues.apache.org/jira/browse/IO-559
 *     - https://github.com/apache/commons-io/commit/2736b6fe
 *   - If path is relative like `../` or starts with `~/` or `~user/`, then `null` will be returned,
 *     as unknown path components cannot be removed from a path.
 * - The path separator `/` will not be added to home `~` and `~user` if it doesn't already
 *   exist like [`org.apache.commons.io.FilenameUtils#normalize(String)`] does.
 *   See also `getUnixPathPrefixLength(String)` in `FileUtils.java`.
 *
 * Following examples assume `keepEndSeparator` is `false` and `removeDoubleDot` is `true`.
 * Check `runNormalizeTests()` in `FileUtilsTestsProvider.java` for more examples.
 *
 * ```
 * null -> null
 * `` -> null
 * `/` -> `/`
 *
 * `:` -> `:`
 * `1:/a/b/c.txt` -> `1:/a/b/c.txt`
 * `///a//b///c/d///` -> `/a/b/c/d`
 * `/foo//` -> `/foo`
 * `/foo/./` -> `/foo`
 * `//foo//./bar` -> `/foo/bar`
 * `~` -> `~`
 * `~/` -> `~`
 * `~user/` -> `~user`
 * `~user` -> `~user`
 * `~user/a` -> `~user/a`
 *
 * `.` -> null
 * `./` -> null
 * `/.` -> `/`
 * `/./` -> `/`
 * `././` -> null
 * `/./.` -> `/`
 * `/././a` -> `/a`
 * `/./b/./` -> `/b`
 * `././c/./` -> `c`
 *
 * `..` -> null
 * `../` -> null
 * `/..` -> `/`
 * `/../` -> `/`
 * `/../a` -> `/a`
 * `../a/.` -> null
 * `../a` -> null
 * `/a/b/..` -> `/a`
 * `/a/b/../..` -> `/`
 * `/a/b/../../..` -> `/`
 * `/a/b/../../../c` -> `/c`
 * `./../a` -> null
 * `././../a` -> null
 * `././.././a` -> null
 * `a/../../b` -> null
 * `a/./../b` -> `b`
 * `a/./.././b` -> `b`
 * `foo/../../bar` -> null
 * `/foo/../bar` -> `/bar`
 * `/foo/../bar/` -> `/bar`
 * `/foo/../bar/../baz` -> `/baz`
 * `foo/bar/..` -> `foo`
 * `foo/../bar` -> `bar`
 * `~/..` -> null
 * `~/../` -> null
 * `~/../a/.` -> null
 * `~/..` -> null
 * `~/foo/../bar/` -> `~/bar`
 * `C:/foo/../bar` -> `C:/bar`
 * `//server/foo/../bar` -> `/server/bar`
 * `//server/../bar` -> `/bar`
 * ```
 *
 * [`org.apache.commons.io.FilenameUtils#normalize(String)`]: https://commons.apache.org/proper/commons-io/apidocs/org/apache/commons/io/FilenameUtils.html#normalize-java.lang.String-
 *
 * @param path The `path` to normalize.
 * @param keepEndSeparator Whether to keep path separator `/` at end if it exists.
 * @param removeDoubleDot If set to `true`. then double dots `..` will be removed along with the
 *                        parent directory for each `..` component.
 * @return Returns the `path` passed as `normalized path`.
 */
char* normalize_path(char* path, bool keepEndSeparator, bool removeDoubleDot);



/**
 * Remove consecutive duplicate path separators `//` and the trailing path separator if not
 * rootfs `/`.
 *
 * @param path The `path` to remove from.
 * @param keepEndSeparator Whether to keep path separator `/` at end if it exists.
 * @return Returns the path with consecutive duplicate path separators removed.
 */
char* remove_dup_separator(char* path, bool keepEndSeparator);





/**
 * Check whether the `path` is in `dir_path`.
 *
 * @param label The label for errors.
 * @param path The `path` to check.
 * @param dir_path The `directory path` to check in. This must be an absolute path.
 * @param ensure_under If set to `true`, then it will be ensured that `path` is under the directory
 *                     and does not equal it. If set to `false`, it can either equal the directory
 *                     path or be under it.
 * @return Returns `0` if `path` is in `dir_path`, `1` if `path` is not in `dir_path`,
 *         otherwise `-1` on other failures.
 */
bool is_path_in_dir_path(const char* label, const char* path, const char* dir_path, bool ensure_under);

#endif // FILE_UTILS_H

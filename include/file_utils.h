/* Cross-platform file utilities with UTF-8 support */

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdio.h>

/* Cross-platform file open with UTF-8 path support.
 * On Windows: converts UTF-8 path to UTF-16 and uses _wfopen()
 * On Unix/macOS: calls fopen() directly
 *
 * Parameters:
 *   utf8_path - File path in UTF-8 encoding
 *   mode - Standard fopen() mode string ("r", "w", "rb", etc.)
 *
 * Returns:
 *   FILE* on success, NULL on failure
 */
FILE *file_open(const char *utf8_path, const char *mode);

/* Cross-platform file removal with UTF-8 path support.
 * On Windows: converts UTF-8 path to UTF-16 and uses _wremove()
 * On Unix/macOS: calls remove() directly
 *
 * Parameters:
 *   utf8_path - File path in UTF-8 encoding
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int file_remove(const char *utf8_path);

/* Cross-platform directory creation with UTF-8 path support.
 * On Windows: converts UTF-8 path to UTF-16 and uses _wmkdir()
 * On Unix/macOS: calls mkdir() directly with 0755 permissions
 *
 * Parameters:
 *   utf8_path - Directory path in UTF-8 encoding
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int file_mkdir(const char *utf8_path);

/* Resolve a file path by searching XDG data directories.
 * For relative paths (not starting with / ./ ../), searches:
 *   1. Current working directory
 *   2. $XDG_DATA_HOME/bloom-lisp/ (default: ~/.local/share/bloom-lisp/)
 *   3. Each dir in $XDG_DATA_DIRS/bloom-lisp/ (default: /usr/local/share:/usr/share)
 *
 * Parameters:
 *   filename - File path to resolve
 *   resolved - Output buffer for resolved path
 *   resolved_size - Size of output buffer
 *
 * Returns:
 *   resolved on success (points to the found path), NULL if not found anywhere
 */
const char *file_resolve(const char *filename, char *resolved, size_t resolved_size);

#ifdef _WIN32
#include <wchar.h>

/* Convert UTF-8 string to UTF-16 (wide char) for Windows APIs.
 * Caller must free the returned buffer with free().
 *
 * Parameters:
 *   utf8_str - String in UTF-8 encoding
 *
 * Returns:
 *   Wide char string on success, NULL on failure
 */
wchar_t *utf8_to_utf16(const char *utf8_str);
#endif

#endif /* FILE_UTILS_H */

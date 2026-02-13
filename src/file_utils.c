/* Cross-platform file utilities with UTF-8 support */

#include "../include/file_utils.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <wchar.h>
#include <windows.h>

/* Convert UTF-8 string to UTF-16 (wide char) for Windows APIs.
 * Caller must free the returned buffer with free().
 *
 * Returns: Wide char string on success, NULL on failure
 */
wchar_t *utf8_to_utf16(const char *utf8_str)
{
    if (!utf8_str)
        return NULL;

    /* Get required buffer size */
    int wchar_count = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (wchar_count == 0)
        return NULL;

    /* Allocate buffer */
    wchar_t *wchar_str = (wchar_t *)malloc(wchar_count * sizeof(wchar_t));
    if (!wchar_str)
        return NULL;

    /* Convert UTF-8 to UTF-16 */
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wchar_str, wchar_count) == 0) {
        free(wchar_str);
        return NULL;
    }

    return wchar_str;
}
#endif

FILE *file_open(const char *utf8_path, const char *mode)
{
    if (!utf8_path || !mode)
        return NULL;

#ifdef _WIN32
    /* Convert UTF-8 path and mode to UTF-16 */
    wchar_t *wpath = utf8_to_utf16(utf8_path);
    if (!wpath)
        return NULL;

    wchar_t *wmode = utf8_to_utf16(mode);
    if (!wmode) {
        free(wpath);
        return NULL;
    }

    /* Open file with wide char API */
    FILE *file = _wfopen(wpath, wmode);

    /* Clean up */
    free(wpath);
    free(wmode);

    return file;
#else
    /* On Unix/macOS, fopen() already handles UTF-8 */
    return fopen(utf8_path, mode);
#endif
}

int file_remove(const char *utf8_path)
{
    if (!utf8_path)
        return -1;

#ifdef _WIN32
    /* Convert UTF-8 path to UTF-16 */
    wchar_t *wpath = utf8_to_utf16(utf8_path);
    if (!wpath)
        return -1;

    /* Remove file with wide char API */
    int result = _wremove(wpath);

    /* Clean up */
    free(wpath);

    return result;
#else
    /* On Unix/macOS, remove() already handles UTF-8 */
    return remove(utf8_path);
#endif
}

static int file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

const char *file_resolve(const char *filename, char *resolved, size_t resolved_size)
{
    if (!filename || !resolved || resolved_size == 0)
        return NULL;

    /* Absolute or explicitly relative paths: use as-is */
    if (filename[0] == '/' || (filename[0] == '.' && (filename[1] == '/' || filename[1] == '.')))
        return filename;

    /* Try relative to cwd first */
    if (file_exists(filename))
        return filename;

    /* Try XDG_DATA_HOME/bloom-lisp/ */
    const char *data_home = getenv("XDG_DATA_HOME");
    if (data_home && data_home[0]) {
        snprintf(resolved, resolved_size, "%s/bloom-lisp/%s", data_home, filename);
        if (file_exists(resolved))
            return resolved;
    } else {
        const char *home = getenv("HOME");
        if (home) {
            snprintf(resolved, resolved_size, "%s/.local/share/bloom-lisp/%s", home, filename);
            if (file_exists(resolved))
                return resolved;
        }
    }

    /* Try each dir in XDG_DATA_DIRS/bloom-lisp/ */
    const char *data_dirs = getenv("XDG_DATA_DIRS");
    if (!data_dirs || !data_dirs[0])
        data_dirs = "/usr/local/share:/usr/share";

    /* Copy so we can tokenize */
    size_t dirs_len = strlen(data_dirs);
    char *dirs_copy = malloc(dirs_len + 1);
    if (!dirs_copy)
        return NULL;
    memcpy(dirs_copy, data_dirs, dirs_len + 1);

    char *saveptr = NULL;
    char *dir = strtok_r(dirs_copy, ":", &saveptr);
    while (dir) {
        snprintf(resolved, resolved_size, "%s/bloom-lisp/%s", dir, filename);
        if (file_exists(resolved)) {
            free(dirs_copy);
            return resolved;
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }
    free(dirs_copy);

    /* Not found anywhere, return original (will fail at open) */
    return filename;
}

int file_mkdir(const char *utf8_path)
{
    if (!utf8_path)
        return -1;

#ifdef _WIN32
    /* Convert UTF-8 path to UTF-16 */
    wchar_t *wpath = utf8_to_utf16(utf8_path);
    if (!wpath)
        return -1;

    /* Create directory with wide char API */
    int result = _wmkdir(wpath);

    /* Clean up */
    free(wpath);

    return result;
#else
    /* On Unix/macOS, mkdir() already handles UTF-8 */
    return mkdir(utf8_path, 0755);
#endif
}

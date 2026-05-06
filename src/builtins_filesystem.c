#include "builtins_internal.h"

/* Get user's home directory path (cross-platform)
 * Unix/Linux/macOS: $HOME
 * Windows: %USERPROFILE% or %HOMEDRIVE%%HOMEPATH%
 * Returns: String with home directory or NIL if not found
 */
static LispObject *builtin_home_directory(LispObject *args, Environment *env)
{
    (void)args; /* Takes no arguments */
    (void)env;

    const char *home = NULL;

#if defined(_WIN32) || defined(_WIN64)
    /* Windows: Try USERPROFILE first */
    home = getenv("USERPROFILE");

    /* Fallback: HOMEDRIVE + HOMEPATH */
    if (home == NULL) {
        const char *homedrive = getenv("HOMEDRIVE");
        const char *homepath = getenv("HOMEPATH");

        if (homedrive != NULL && homepath != NULL) {
            size_t len = strlen(homedrive) + strlen(homepath) + 1;
            char *combined = GC_malloc(len);
            snprintf(combined, len, "%s%s", homedrive, homepath);
            home = combined;
        }
    }
#else
    /* Unix/Linux/macOS: Use HOME */
    home = getenv("HOME");
#endif

    if (home == NULL) {
        return NIL; /* No home directory found */
    }

    return lisp_make_string(home);
}

/* Expand ~/ in file paths to home directory (cross-platform)
 * Takes: String (file path)
 * Returns: String (expanded path) or original if no ~/ prefix
 * Example: (expand-path "~/config.lisp") => "/home/user/config.lisp"
 */
static LispObject *builtin_expand_path(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("expand-path");

    LispObject *path_obj = lisp_car(args);
    if (LISP_TYPE(path_obj) != LISP_STRING) {
        return lisp_make_error("expand-path requires a string argument");
    }

    const char *path = LISP_STR_VAL(path_obj);

    /* Check if path starts with ~/ */
    if (path[0] != '~' || (path[1] != '/' && path[1] != '\\' && path[1] != '\0')) {
        /* Not a ~/ path - return original */
        return path_obj;
    }

    /* Get home directory */
    LispObject *home_obj = builtin_home_directory(NIL, env);
    if (home_obj == NIL || LISP_TYPE(home_obj) != LISP_STRING) {
        /* No home directory - return error */
        return lisp_make_error("expand-path: cannot determine home directory");
    }

    const char *home = LISP_STR_VAL(home_obj);

    /* Calculate expanded path length */
    /* If path is just "~", use home directory directly */
    if (path[1] == '\0') {
        return home_obj;
    }

    /* Skip ~/ or ~\ */
    const char *rest = path + 2;

    /* Build expanded path: home + / + rest */
    size_t home_len = strlen(home);
    size_t rest_len = strlen(rest);
    size_t total_len = home_len + 1 + rest_len + 1;

    char *expanded = GC_malloc(total_len);

#if defined(_WIN32) || defined(_WIN64)
    /* Windows: Use backslash separator */
    snprintf(expanded, total_len, "%s\\%s", home, rest);
#else
    /* Unix: Use forward slash separator */
    snprintf(expanded, total_len, "%s/%s", home, rest);
#endif

    return lisp_make_string(expanded);
}

/* Read an environment variable.
 * Takes: String (variable name)
 * Returns: String (value) or nil if not set
 */
static LispObject *builtin_getenv(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("getenv requires 1 argument");

    LispObject *name = lisp_car(args);
    if (LISP_TYPE(name) != LISP_STRING)
        return lisp_make_error("getenv requires a string argument");

    const char *value = getenv(LISP_STR_VAL(name));
    if (!value)
        return NIL;

    return lisp_make_string(value);
}

/* Return platform-specific user data directory for an application.
 * Takes: String (app name)
 * Returns: String (path, not created) or error
 * Unix: $XDG_DATA_HOME/app or ~/.local/share/app
 * Windows: %LOCALAPPDATA%\app or %APPDATA%\app
 */
static LispObject *builtin_data_directory(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("data-directory requires 1 argument");

    LispObject *app = lisp_car(args);
    if (LISP_TYPE(app) != LISP_STRING)
        return lisp_make_error("data-directory requires a string argument");

    const char *app_name = LISP_STR_VAL(app);
    char path[PATH_MAX];

#if defined(_WIN32) || defined(_WIN64)
    const char *dir = getenv("LOCALAPPDATA");
    if (!dir)
        dir = getenv("APPDATA");
    if (!dir)
        return lisp_make_error("data-directory: cannot determine data directory");
    snprintf(path, sizeof(path), "%s\\%s", dir, app_name);
#else
    const char *dir = getenv("XDG_DATA_HOME");
    if (dir && dir[0]) {
        snprintf(path, sizeof(path), "%s/%s", dir, app_name);
    } else {
        const char *home = getenv("HOME");
        if (!home)
            return lisp_make_error("data-directory: cannot determine home directory");
        snprintf(path, sizeof(path), "%s/.local/share/%s", home, app_name);
    }
#endif

    return lisp_make_string(path);
}

/* Return platform-specific user config directory for an application.
 * Takes: String (app name)
 * Returns: String (path, not created) or error
 * Unix: $XDG_CONFIG_HOME/app or ~/.config/app
 * Windows: %APPDATA%\app
 */
static LispObject *builtin_config_directory(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("config-directory requires 1 argument");

    LispObject *app = lisp_car(args);
    if (LISP_TYPE(app) != LISP_STRING)
        return lisp_make_error("config-directory requires a string argument");

    const char *app_name = LISP_STR_VAL(app);
    char path[PATH_MAX];

#if defined(_WIN32) || defined(_WIN64)
    const char *dir = getenv("APPDATA");
    if (!dir)
        return lisp_make_error("config-directory: cannot determine config directory");
    snprintf(path, sizeof(path), "%s\\%s", dir, app_name);
#else
    const char *dir = getenv("XDG_CONFIG_HOME");
    if (dir && dir[0]) {
        snprintf(path, sizeof(path), "%s/%s", dir, app_name);
    } else {
        const char *home = getenv("HOME");
        if (!home)
            return lisp_make_error("config-directory: cannot determine home directory");
        snprintf(path, sizeof(path), "%s/.config/%s", home, app_name);
    }
#endif

    return lisp_make_string(path);
}

/* Check if a file or directory exists.
 * Takes: String (path)
 * Returns: #t or nil
 */
static LispObject *builtin_file_exists_question(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("file-exists? requires 1 argument");

    LispObject *path = lisp_car(args);
    if (LISP_TYPE(path) != LISP_STRING)
        return lisp_make_error("file-exists? requires a string argument");

    return file_exists(LISP_STR_VAL(path)) ? LISP_TRUE : NIL;
}

/* Create directory and all parents (mkdir -p).
 * Takes: String (path)
 * Returns: #t or error
 */
static LispObject *builtin_mkdir(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("mkdir requires 1 argument");

    LispObject *path = lisp_car(args);
    if (LISP_TYPE(path) != LISP_STRING)
        return lisp_make_error("mkdir requires a string argument");

    if (file_exists(LISP_STR_VAL(path)))
        return LISP_TRUE;

    if (file_mkdir_p(LISP_STR_VAL(path)) != 0) {
        char errbuf[256];
        snprintf(errbuf, sizeof(errbuf), "mkdir: %s: %s", LISP_STR_VAL(path),
                 strerror(errno));
        return lisp_make_error(errbuf);
    }

    return LISP_TRUE;
}

void register_filesystem_builtins(Environment *env)
{
    REGISTER("home-directory", builtin_home_directory);
    REGISTER("expand-path", builtin_expand_path);
    REGISTER("getenv", builtin_getenv);
    REGISTER("data-directory", builtin_data_directory);
    REGISTER("config-directory", builtin_config_directory);
    REGISTER("file-exists?", builtin_file_exists_question);
    REGISTER("mkdir", builtin_mkdir);
}

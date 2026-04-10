#include "builtins_internal.h"

/* Helper function to create a file stream object */
LispObject *lisp_make_file_stream(FILE *file)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_FILE_STREAM;
    obj->value.file = file;
    return obj;
}

static LispObject *builtin_open(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_error("open requires at least 1 argument");
    }

    LispObject *filename_obj = lisp_car(args);
    if (filename_obj->type != LISP_STRING) {
        return lisp_make_error("open requires a string filename");
    }

    /* Default mode is "r" (read) */
    const char *mode = "r";
    if (lisp_cdr(args) != NIL && lisp_cdr(args) != NULL) {
        LispObject *mode_obj = lisp_car(lisp_cdr(args));
        if (mode_obj->type != LISP_STRING) {
            return lisp_make_error("open mode must be a string");
        }
        mode = mode_obj->value.string;
    }

    FILE *file = file_open(filename_obj->value.string, mode);
    if (file == NULL) {
        char error[512];
        snprintf(error, sizeof(error), "open: cannot open file '%s'", filename_obj->value.string);
        return lisp_make_error(error);
    }

    return lisp_make_file_stream(file);
}

static LispObject *builtin_close(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("close");

    LispObject *stream_obj = lisp_car(args);
    if (stream_obj->type != LISP_FILE_STREAM) {
        return lisp_make_error("close requires a file stream");
    }

    if (stream_obj->value.file != NULL) {
        fclose(stream_obj->value.file);
        stream_obj->value.file = NULL;
    }

    return NIL;
}

static LispObject *builtin_read_line(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("read-line");

    LispObject *stream_obj = lisp_car(args);
    if (stream_obj->type != LISP_FILE_STREAM) {
        return lisp_make_error("read-line requires a file stream");
    }

    FILE *file = stream_obj->value.file;
    if (file == NULL) {
        return lisp_make_error("read-line: file is closed");
    }

    /* Read line with dynamic buffer */
    size_t size = 256;
    char *buffer = GC_malloc(size);
    size_t pos = 0;

    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            /* Unix line ending */
            break;
        } else if (c == '\r') {
            /* Check for Windows (\r\n) or old Mac (\r) line ending */
            int next_c = fgetc(file);
            if (next_c == '\n') {
                /* Windows line ending \r\n */
                break;
            } else if (next_c != EOF) {
                /* Old Mac line ending \r, put back the character */
                ungetc(next_c, file);
            }
            /* We got \r, now break */
            break;
        }

        if (pos >= size - 1) {
            size *= 2;
            char *new_buffer = GC_malloc(size);
            strncpy(new_buffer, buffer, pos);
            buffer = new_buffer;
        }

        buffer[pos++] = c;
    }

    /* Check for EOF without newline */
    if (pos == 0 && c == EOF) {
        return NIL; /* End of file */
    }

    buffer[pos] = '\0';
    return lisp_make_string(buffer);
}

static LispObject *builtin_write_line(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_error("write-line requires at least 1 argument");
    }

    LispObject *stream_obj = lisp_car(args);
    if (stream_obj->type != LISP_FILE_STREAM) {
        return lisp_make_error("write-line requires a file stream");
    }

    FILE *file = stream_obj->value.file;
    if (file == NULL) {
        return lisp_make_error("write-line: file is closed");
    }

    LispObject *rest = lisp_cdr(args);
    if (rest == NIL) {
        return lisp_make_error("write-line requires a string to write");
    }

    LispObject *text_obj = lisp_car(rest);
    if (text_obj->type != LISP_STRING) {
        return lisp_make_error("write-line requires a string");
    }

    fprintf(file, "%s\n", text_obj->value.string);
    fflush(file);

    return text_obj;
}

/* Read S-expressions from file */
static LispObject *builtin_read_sexp(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("read-sexp");

    LispObject *arg = lisp_car(args);
    FILE *file = NULL;
    int should_close = 0;

    /* Check if argument is file stream or filename */
    if (arg->type == LISP_FILE_STREAM) {
        file = arg->value.file;
        if (file == NULL) {
            return lisp_make_error("read-sexp: file is closed");
        }
    } else if (arg->type == LISP_STRING) {
        /* Open file */
        file = file_open(arg->value.string, "r");
        if (file == NULL) {
            char error[512];
            snprintf(error, sizeof(error), "read-sexp: cannot open file '%s'", arg->value.string);
            return lisp_make_error(error);
        }
        should_close = 1;
    } else {
        return lisp_make_error("read-sexp requires a filename (string) or file stream");
    }

    /* Read entire file */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        if (should_close) {
            fclose(file);
        }
        return lisp_make_error("read-sexp: cannot determine file size");
    }

    char *buffer = GC_malloc(size + 1);
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';

    if (should_close) {
        fclose(file);
    }

    /* Parse all expressions */
    const char *input = buffer;
    LispObject *result = NIL;
    LispObject *last = NIL;

    while (*input) {
        /* Skip whitespace and comments */
        while (*input == ' ' || *input == '\t' || *input == '\n' || *input == '\r' || *input == ';') {
            if (*input == ';') {
                while (*input && *input != '\n') {
                    input++;
                }
            } else {
                input++;
            }
        }

        if (*input == '\0') {
            break;
        }

        /* Parse expression */
        LispObject *expr = lisp_read(&input);
        if (expr == NULL) {
            break;
        }

        if (expr->type == LISP_ERROR) {
            return expr;
        }

        LIST_APPEND(result, last, expr);
    }

    /* Return single expression if only one, otherwise return list */
    if (result != NIL && lisp_cdr(result) == NIL) {
        return lisp_car(result);
    }

    return result;
}

/* Read JSON from file - basic JSON parser */
static LispObject *builtin_read_json(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("read-json");

    LispObject *arg = lisp_car(args);
    FILE *file = NULL;
    int should_close = 0;

    /* Check if argument is file stream or filename */
    if (arg->type == LISP_FILE_STREAM) {
        file = arg->value.file;
        if (file == NULL) {
            return lisp_make_error("read-json: file is closed");
        }
    } else if (arg->type == LISP_STRING) {
        /* Open file */
        file = file_open(arg->value.string, "r");
        if (file == NULL) {
            char error[512];
            snprintf(error, sizeof(error), "read-json: cannot open file '%s'", arg->value.string);
            return lisp_make_error(error);
        }
        should_close = 1;
    } else {
        return lisp_make_error("read-json requires a filename (string) or file stream");
    }

    /* Read entire file */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        if (should_close) {
            fclose(file);
        }
        return lisp_make_error("read-json: cannot determine file size");
    }

    char *buffer = GC_malloc(size + 1);
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';

    if (should_close) {
        fclose(file);
    }

    /* Simple JSON parser - this is a basic implementation */
    /* Skip whitespace */
    const char *p = buffer;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
        p++;
    }

    if (*p == '\0') {
        return NIL;
    }

    /* Parse JSON value */
    LispObject *result = NULL;
    if (*p == '{') {
        /* JSON object -> hash table */
        result = lisp_make_hash_table();
        p++; /* Skip '{' */
        while (*p && *p != '}') {
            /* Skip whitespace */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
                p++;
            }
            if (*p == '}') {
                break;
            }

            /* Parse key (must be string) */
            if (*p != '"') {
                return lisp_make_error("read-json: object key must be a string");
            }
            p++; /* Skip '"' */
            const char *key_start = p;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p + 1)) {
                    p += 2; /* Skip escape sequence */
                } else {
                    p++;
                }
            }
            if (*p != '"') {
                return lisp_make_error("read-json: unterminated string");
            }
            size_t key_len = p - key_start;
            char *key = GC_malloc(key_len + 1);
            memcpy(key, key_start, key_len);
            key[key_len] = '\0';
            p++; /* Skip '"' */

            /* Skip whitespace */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
                p++;
            }
            if (*p != ':') {
                return lisp_make_error("read-json: expected ':' after key");
            }
            p++; /* Skip ':' */

            /* Skip whitespace */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
                p++;
            }

            /* Parse value (recursive) */
            LispObject *json_value = NULL;
            if (*p == '"') {
                /* String */
                p++;
                const char *val_start = p;
                while (*p && *p != '"') {
                    if (*p == '\\' && *(p + 1)) {
                        p += 2;
                    } else {
                        p++;
                    }
                }
                if (*p != '"') {
                    return lisp_make_error("read-json: unterminated string");
                }
                size_t val_len = p - val_start;
                char *val_str = GC_malloc(val_len + 1);
                memcpy(val_str, val_start, val_len);
                val_str[val_len] = '\0';
                json_value = lisp_make_string(val_str);
                p++; /* Skip '"' */
            } else if (*p == '{') {
                /* Nested object - simplified: return error for now */
                return lisp_make_error("read-json: nested objects not yet supported");
            } else if (*p == '[') {
                /* Array - simplified: return error for now */
                return lisp_make_error("read-json: arrays not yet supported");
            } else if (strncmp(p, "true", 4) == 0) {
                json_value = lisp_make_boolean(1);
                p += 4;
            } else if (strncmp(p, "false", 5) == 0) {
                json_value = NIL;
                p += 5;
            } else if (strncmp(p, "null", 4) == 0) {
                json_value = NIL;
                p += 4;
            } else if (isdigit(*p) || *p == '-') {
                /* Number - simplified parsing */
                const char *num_start = p;
                while (*p && (isdigit(*p) || *p == '.' || *p == '-' || *p == '+' || *p == 'e' || *p == 'E')) {
                    p++;
                }
                size_t num_len = p - num_start;
                char *num_str = GC_malloc(num_len + 1);
                memcpy(num_str, num_start, num_len);
                num_str[num_len] = '\0';
                if (strchr(num_str, '.') != NULL) {
                    json_value = lisp_make_number(atof(num_str));
                } else {
                    json_value = lisp_make_integer(atoll(num_str));
                }
            } else {
                return lisp_make_error("read-json: unexpected character");
            }

            /* Store in hash table */
            hash_table_set_entry(result, lisp_make_string(key), json_value);

            /* Skip whitespace */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
                p++;
            }
            if (*p == ',') {
                p++; /* Skip ',' */
            } else if (*p != '}') {
                return lisp_make_error("read-json: expected ',' or '}'");
            }
        }
        if (*p == '}') {
            p++;
        }
    } else if (*p == '"') {
        /* JSON string */
        p++;
        const char *str_start = p;
        while (*p && *p != '"') {
            if (*p == '\\' && *(p + 1)) {
                p += 2;
            } else {
                p++;
            }
        }
        if (*p != '"') {
            return lisp_make_error("read-json: unterminated string");
        }
        size_t str_len = p - str_start;
        char *str = GC_malloc(str_len + 1);
        memcpy(str, str_start, str_len);
        str[str_len] = '\0';
        result = lisp_make_string(str);
        p++;
    } else if (strncmp(p, "true", 4) == 0) {
        result = lisp_make_boolean(1);
    } else if (strncmp(p, "false", 5) == 0) {
        result = NIL;
    } else if (strncmp(p, "null", 4) == 0) {
        result = NIL;
    } else if (isdigit(*p) || *p == '-') {
        /* Number */
        const char *num_start = p;
        while (*p && (isdigit(*p) || *p == '.' || *p == '-' || *p == '+' || *p == 'e' || *p == 'E')) {
            p++;
        }
        size_t num_len = p - num_start;
        char *num_str = GC_malloc(num_len + 1);
        memcpy(num_str, num_start, num_len);
        num_str[num_len] = '\0';
        if (strchr(num_str, '.') != NULL) {
            result = lisp_make_number(atof(num_str));
        } else {
            result = lisp_make_integer(atoll(num_str));
        }
    } else {
        return lisp_make_error("read-json: unsupported JSON value type");
    }

    return result;
}

/* Delete a file */
static LispObject *builtin_delete_file(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("delete-file");

    LispObject *filename_obj = lisp_car(args);
    if (filename_obj->type != LISP_STRING) {
        return lisp_make_error("delete-file requires a string filename");
    }

    /* Attempt to delete the file */
    if (file_remove(filename_obj->value.string) == 0) {
        /* Success */
        return NIL;
    } else {
        /* Failure - return error with errno message */
        char error[512];
        snprintf(error, sizeof(error), "delete-file: failed to delete '%s': %s", filename_obj->value.string,
                 strerror(errno));
        return lisp_make_error(error);
    }
}

/* Load and evaluate a Lisp file */
static LispObject *builtin_load(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("load");

    LispObject *filename_obj = lisp_car(args);
    if (filename_obj->type != LISP_STRING) {
        return lisp_make_error("load requires a string filename");
    }

    /* Save current *package* and restore after load */
    LispObject *saved_pkg = env_lookup(env, sym_star_package_star->value.symbol);

    LispObject *result = lisp_load_file(filename_obj->value.string, env);

    /* Restore *package* */
    if (saved_pkg) {
        env_set(env, sym_star_package_star->value.symbol, saved_pkg);
    }

    /* Return the result of the last expression evaluated, or nil if error */
    if (result && result->type == LISP_ERROR) {
        return result;
    }

    /* Return the last evaluated expression, or nil if file was empty */
    return result ? result : NIL;
}

static LispObject *builtin_read_file_raw(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("read-file-raw");

    LispObject *filename_obj = lisp_car(args);
    if (filename_obj->type != LISP_STRING) {
        return lisp_make_error("read-file-raw requires a string filename");
    }

    FILE *file = file_open(filename_obj->value.string, "rb");
    if (file == NULL) {
        char error[512];
        snprintf(error, sizeof(error), "read-file-raw: cannot open file '%s'",
                 filename_obj->value.string);
        return lisp_make_error(error);
    }

    /* Determine file size */
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return lisp_make_error("read-file-raw: fseek failed");
    }
    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return lisp_make_error("read-file-raw: ftell failed");
    }
    rewind(file);

    /* Read all bytes verbatim */
    char *buffer = GC_malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return lisp_make_error("read-file-raw: allocation failed");
    }
    size_t read = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    if (read != (size_t)size) {
        return lisp_make_error("read-file-raw: short read");
    }
    buffer[size] = '\0';

    return lisp_make_string(buffer);
}

void register_file_io_builtins(Environment *env)
{
    REGISTER("open", builtin_open);
    REGISTER("close", builtin_close);
    REGISTER("read-line", builtin_read_line);
    REGISTER("write-line", builtin_write_line);
    REGISTER("read-sexp", builtin_read_sexp);
    REGISTER("read-json", builtin_read_json);
    REGISTER("read-file-raw", builtin_read_file_raw);
    REGISTER("delete-file", builtin_delete_file);
    REGISTER("load", builtin_load);
}

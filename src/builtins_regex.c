#include "builtins_internal.h"

/* Resolve a pattern argument that may be a string or a precompiled regex.
 * On success returns NULL and sets *re_out. On failure returns an error
 * LispObject. The caller owns *re_out iff pattern_obj->type != LISP_REGEX. */
static LispObject *resolve_pattern(LispObject *pattern_obj, const char *name,
                                   pcre2_code **re_out)
{
    if (pattern_obj->type == LISP_REGEX) {
        *re_out = pattern_obj->value.regex.code;
        return NULL;
    }
    if (pattern_obj->type != LISP_STRING) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: pattern must be a string or compiled regex", name);
        return lisp_make_error(buf);
    }
    char *error_msg = NULL;
    *re_out = compile_regex_pattern(pattern_obj->value.string, &error_msg);
    if (*re_out == NULL) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s: %s", name, error_msg ? error_msg : "invalid pattern");
        return lisp_make_error(buf);
    }
    return NULL;
}

/* Regex helper: extract pattern + string args and resolve the pattern to a
 * pcre2_code. Returns NULL on success (sets *pattern_obj_out, *string_out,
 * *re_out). Returns error LispObject on failure. The caller owns *re_out iff
 * (*pattern_obj_out)->type != LISP_REGEX. */
static LispObject *regex_compile_args(LispObject *args, const char *name,
                                      LispObject **pattern_obj_out,
                                      LispObject **string_out,
                                      pcre2_code **re_out)
{
    if (args == NIL || lisp_cdr(args) == NIL) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s requires 2 arguments", name);
        return lisp_make_error(buf);
    }
    *pattern_obj_out = lisp_car(args);
    *string_out = lisp_car(lisp_cdr(args));
    if ((*string_out)->type != LISP_STRING) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: subject must be a string", name);
        return lisp_make_error(buf);
    }
    return resolve_pattern(*pattern_obj_out, name, re_out);
}

static LispObject *builtin_regex_match_question(LispObject *args, Environment *env)
{
    (void)env;
    LispObject *pattern_obj, *string_obj;
    pcre2_code *re;
    LispObject *err = regex_compile_args(args, "regex-match?", &pattern_obj, &string_obj, &re);
    if (err)
        return err;

    pcre2_match_data *match_data = execute_regex(re, string_obj->value.string);
    int result = (match_data != NULL);
    if (match_data)
        pcre2_match_data_free(match_data);
    if (pattern_obj->type != LISP_REGEX)
        pcre2_code_free(re);
    return result ? LISP_TRUE : NIL;
}

static LispObject *builtin_regex_find(LispObject *args, Environment *env)
{
    (void)env;
    LispObject *pattern_obj, *string_obj;
    pcre2_code *re;
    LispObject *err = regex_compile_args(args, "regex-find", &pattern_obj, &string_obj, &re);
    if (err)
        return err;

    pcre2_match_data *match_data = execute_regex(re, string_obj->value.string);
    LispObject *result = NIL;
    if (match_data != NULL) {
        char *matched = extract_capture(match_data, string_obj->value.string, 0);
        if (matched)
            result = lisp_make_string(matched);
        pcre2_match_data_free(match_data);
    }
    if (pattern_obj->type != LISP_REGEX)
        pcre2_code_free(re);
    return result;
}

static LispObject *builtin_regex_find_all(LispObject *args, Environment *env)
{
    (void)env;
    LispObject *pattern_obj, *string_obj;
    pcre2_code *re;
    LispObject *err = regex_compile_args(args, "regex-find-all", &pattern_obj, &string_obj, &re);
    if (err)
        return err;

    LispObject *result = NIL;
    LispObject *tail = NULL;

    const char *subject = string_obj->value.string;
    size_t offset = 0;
    size_t subject_len = strlen(subject);

    while (offset < subject_len) {
        pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

        int rc = pcre2_match(re, (PCRE2_SPTR)subject, subject_len, offset, 0, match_data, NULL);

        if (rc < 0) {
            pcre2_match_data_free(match_data);
            break;
        }

        char *matched = extract_capture(match_data, subject, 0);
        if (matched)
            LIST_APPEND(result, tail, lisp_make_string(matched));

        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        offset = ovector[1];

        pcre2_match_data_free(match_data);

        if (offset == ovector[0]) {
            offset++; /* Avoid infinite loop on zero-length match */
        }
    }

    if (pattern_obj->type != LISP_REGEX)
        pcre2_code_free(re);

    return result;
}

static LispObject *builtin_regex_extract(LispObject *args, Environment *env)
{
    (void)env;
    LispObject *pattern_obj, *string_obj;
    pcre2_code *re;
    LispObject *err = regex_compile_args(args, "regex-extract", &pattern_obj, &string_obj, &re);
    if (err)
        return err;

    pcre2_match_data *match_data = execute_regex(re, string_obj->value.string);
    LispObject *result = NIL;
    if (match_data != NULL) {
        int capture_count = get_capture_count(re);
        LispObject *tail = NULL;
        for (int i = 1; i <= capture_count; i++) {
            char *captured = extract_capture(match_data, string_obj->value.string, i);
            if (captured)
                LIST_APPEND(result, tail, lisp_make_string(captured));
        }
        pcre2_match_data_free(match_data);
    }
    if (pattern_obj->type != LISP_REGEX)
        pcre2_code_free(re);
    return result;
}

static LispObject *builtin_regex_replace(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_3("regex-replace");

    LispObject *pattern_obj = lisp_car(args);
    LispObject *string_obj = lisp_car(lisp_cdr(args));
    LispObject *replacement_obj = lisp_car(lisp_cdr(lisp_cdr(args)));

    if (string_obj->type != LISP_STRING || replacement_obj->type != LISP_STRING)
        return lisp_make_error("regex-replace: subject and replacement must be strings");

    pcre2_code *re;
    LispObject *err = resolve_pattern(pattern_obj, "regex-replace", &re);
    if (err)
        return err;

    size_t output_buffer_size = strlen(string_obj->value.string) * 2 + 256;
    size_t output_len = output_buffer_size;
    PCRE2_UCHAR *output = GC_malloc(output_buffer_size + 1); // +1 for null terminator

    int rc = pcre2_substitute(re, (PCRE2_SPTR)string_obj->value.string, PCRE2_ZERO_TERMINATED, 0, /* start offset */
                              PCRE2_SUBSTITUTE_GLOBAL,                                            /* options - replace all */
                              NULL,                                                               /* match data */
                              NULL,                                                               /* match context */
                              (PCRE2_SPTR)replacement_obj->value.string, PCRE2_ZERO_TERMINATED, output, &output_len);

    if (pattern_obj->type != LISP_REGEX)
        pcre2_code_free(re);

    if (rc < 0) {
        char error[256];
        snprintf(error, sizeof(error), "regex-replace: substitution failed (error code: %d)", rc);
        return lisp_make_error(error);
    }

    /* Ensure null termination */
    output[output_len] = '\0';

    return lisp_make_string((char *)output);
}

static LispObject *builtin_regex_replace_all(LispObject *args, Environment *env)
{
    /* Same as regex-replace since we use PCRE2_SUBSTITUTE_GLOBAL */
    return builtin_regex_replace(args, env);
}

static LispObject *builtin_regex_split(LispObject *args, Environment *env)
{
    (void)env;
    LispObject *pattern_obj, *string_obj;
    pcre2_code *re;
    LispObject *err = regex_compile_args(args, "regex-split", &pattern_obj, &string_obj, &re);
    if (err)
        return err;

    LispObject *result = NIL;
    LispObject *tail = NULL;
    const char *subject = string_obj->value.string;
    size_t offset = 0, last_end = 0;
    size_t subject_len = strlen(subject);

    while (offset <= subject_len) {
        pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);
        int rc = pcre2_match(re, (PCRE2_SPTR)subject, subject_len, offset, 0, match_data, NULL);
        if (rc < 0) {
            pcre2_match_data_free(match_data);
            break;
        }

        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        size_t match_start = ovector[0];
        size_t match_end = ovector[1];

        size_t part_len = match_start - last_end;
        char *part = GC_malloc(part_len + 1);
        strncpy(part, subject + last_end, part_len);
        part[part_len] = '\0';
        LIST_APPEND(result, tail, lisp_make_string(part));

        last_end = match_end;
        offset = match_end;
        pcre2_match_data_free(match_data);
        if (offset == match_start)
            offset++;
    }

    if (last_end <= subject_len) {
        char *part = GC_malloc(subject_len - last_end + 1);
        strcpy(part, subject + last_end);
        LIST_APPEND(result, tail, lisp_make_string(part));
    }

    if (pattern_obj->type != LISP_REGEX)
        pcre2_code_free(re);
    return result;
}

static LispObject *builtin_regex_escape(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("regex-escape");
    LispObject *string_obj = lisp_car(args);
    if (string_obj->type != LISP_STRING)
        return lisp_make_error("regex-escape requires a string");

    const char *str = string_obj->value.string;
    size_t len = strlen(str);
    char *escaped = GC_malloc(len * 2 + 1);
    size_t j = 0;

    const char *special = ".^$*+?()[]{}|\\";

    for (size_t i = 0; i < len; i++) {
        if (strchr(special, str[i])) {
            escaped[j++] = '\\';
        }
        escaped[j++] = str[i];
    }
    escaped[j] = '\0';

    return lisp_make_string(escaped);
}

static LispObject *builtin_regex_valid_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("regex-valid?");
    LispObject *pattern_obj = lisp_car(args);

    if (pattern_obj->type == LISP_REGEX)
        return LISP_TRUE;

    if (pattern_obj->type != LISP_STRING)
        return lisp_make_error("regex-valid? requires a string or compiled regex");

    char *error_msg = NULL;
    pcre2_code *re = compile_regex_pattern(pattern_obj->value.string, &error_msg);

    if (re == NULL) {
        return NIL;
    }

    pcre2_code_free(re);
    return LISP_TRUE;
}

static LispObject *builtin_regex_compile(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("regex-compile");
    LispObject *p = lisp_car(args);
    if (p->type != LISP_STRING)
        return lisp_make_error("regex-compile requires a string");
    char *error_msg = NULL;
    pcre2_code *re = compile_regex_pattern(p->value.string, &error_msg);
    if (re == NULL) {
        char buf[512];
        snprintf(buf, sizeof(buf), "regex-compile: %s", error_msg ? error_msg : "invalid pattern");
        return lisp_make_error(buf);
    }
    return lisp_make_regex(re);
}

void register_regex_builtins(Environment *env)
{
    REGISTER("regex-compile", builtin_regex_compile);
    REGISTER("regex-match?", builtin_regex_match_question);
    REGISTER("regex-find", builtin_regex_find);
    REGISTER("regex-find-all", builtin_regex_find_all);
    REGISTER("regex-extract", builtin_regex_extract);
    REGISTER("regex-replace", builtin_regex_replace);
    REGISTER("regex-replace-all", builtin_regex_replace_all);
    REGISTER("regex-split", builtin_regex_split);
    REGISTER("regex-escape", builtin_regex_escape);
    REGISTER("regex-valid?", builtin_regex_valid_question);
}

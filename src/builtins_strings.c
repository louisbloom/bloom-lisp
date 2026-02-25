#include "builtins_internal.h"

/* Helper for wildcard matching */
static int match_char_class(const char **pattern, char c)
{
    const char *p = *pattern + 1; /* Skip '[' */
    int negate = 0;
    int match = 0;

    if (*p == '!') {
        negate = 1;
        p++;
    }

    while (*p && *p != ']') {
        if (*(p + 1) == '-' && *(p + 2) != ']' && *(p + 2) != '\0') {
            /* Range */
            if (c >= *p && c <= *(p + 2)) {
                match = 1;
            }
            p += 3;
        } else {
            /* Single character */
            if (c == *p) {
                match = 1;
            }
            p++;
        }
    }

    if (*p == ']') {
        *pattern = p + 1;
    }

    return negate ? !match : match;
}

static int wildcard_match(const char *pattern, const char *str)
{
    while (*pattern && *str) {
        if (*pattern == '*') {
            pattern++;
            if (*pattern == '\0')
                return 1;
            while (*str) {
                if (wildcard_match(pattern, str))
                    return 1;
                str++;
            }
            return 0;
        } else if (*pattern == '?') {
            pattern++;
            str++;
        } else if (*pattern == '[') {
            if (match_char_class(&pattern, *str)) {
                str++;
            } else {
                return 0;
            }
        } else if (*pattern == *str) {
            pattern++;
            str++;
        } else {
            return 0;
        }
    }

    while (*pattern == '*')
        pattern++;
    return (*pattern == '\0' && *str == '\0');
}

static LispObject *builtin_concat(LispObject *args, Environment *env)
{
    (void)env;
    size_t total_len = 0;

    /* Calculate total length */
    LispObject *curr = args;
    while (curr != NIL && curr != NULL) {
        LispObject *arg = lisp_car(curr);
        if (arg->type != LISP_STRING) {
            return lisp_make_error("concat requires strings");
        }
        total_len += strlen(arg->value.string);
        curr = lisp_cdr(curr);
    }

    /* Concatenate */
    char *result = GC_malloc(total_len + 1);
    result[0] = '\0';

    curr = args;
    while (curr != NIL && curr != NULL) {
        LispObject *arg = lisp_car(curr);
        strcat(result, arg->value.string);
        curr = lisp_cdr(curr);
    }

    LispObject *obj = lisp_make_string(result);
    return obj;
}

static LispObject *builtin_number_to_string(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NULL || args == NIL) {
        return lisp_make_error("number->string: expected at least 1 argument");
    }

    /* Parse arguments: number (required), radix (optional) */
    LispObject *num = lisp_car(args);
    LispObject *radix_obj = (lisp_cdr(args) != NIL) ? lisp_car(lisp_cdr(args)) : NIL;

    if (num == NULL || num == NIL) {
        return lisp_make_error("number->string: first argument cannot be nil");
    }

    /* Validate number argument */
    if (num->type != LISP_INTEGER && num->type != LISP_NUMBER) {
        return lisp_make_error("number->string: first argument must be a number");
    }

    int radix = 10; /* default base */

    /* Parse optional radix */
    if (radix_obj != NIL) {
        if (radix_obj->type != LISP_INTEGER) {
            return lisp_make_error("number->string: radix must be an integer");
        }
        radix = (int)radix_obj->value.integer;
        if (radix < 2 || radix > 36) {
            return lisp_make_error("number->string: radix must be between 2 and 36");
        }
    }

    /* Float formatting (only base 10) */
    if (num->type == LISP_NUMBER) {
        if (radix != 10) {
            return lisp_make_error("number->string: floats only supported in base 10");
        }
        char buffer[64];
        double val = num->value.number;
        /* Format with enough precision, then trim trailing zeros but keep ".0" */
        snprintf(buffer, sizeof(buffer), "%.15f", val);
        /* Find decimal point and trim trailing zeros */
        char *dot = strchr(buffer, '.');
        if (dot) {
            char *end = buffer + strlen(buffer) - 1;
            while (end > dot + 1 && *end == '0') {
                *end-- = '\0';
            }
        }
        return lisp_make_string(buffer);
    }

    /* Integer formatting with arbitrary radix */
    long long value = num->value.integer;

    /* Special case: zero */
    if (value == 0) {
        return lisp_make_string("0");
    }

    char buffer[128];
    int pos = 0;
    int negative = (value < 0);

    /* Use unsigned for conversion to handle INT64_MIN correctly
     * (negating INT64_MIN overflows, but unsigned conversion works) */
    unsigned long long uvalue;
    if (negative) {
        /* Cast to unsigned first to avoid undefined behavior with -INT64_MIN */
        uvalue = (unsigned long long)(-(value + 1)) + 1;
    } else {
        uvalue = (unsigned long long)value;
    }

    /* Convert to given radix */
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    char temp[128];
    int temp_pos = 0;

    while (uvalue > 0) {
        temp[temp_pos++] = digits[uvalue % radix];
        uvalue /= radix;
    }

    /* Add sign */
    if (negative) {
        buffer[pos++] = '-';
    }

    /* Reverse digits */
    while (temp_pos > 0) {
        buffer[pos++] = temp[--temp_pos];
    }

    buffer[pos] = '\0';
    return lisp_make_string(buffer);
}

static LispObject *builtin_string_to_number(LispObject *args, Environment *env)
{
    (void)env;

    if (args == NULL || args == NIL) {
        return lisp_make_error("string->number: expected at least 1 argument");
    }

    /* Parse arguments: string (required), radix (optional) */
    LispObject *str_obj = lisp_car(args);
    LispObject *radix_obj = (lisp_cdr(args) != NIL) ? lisp_car(lisp_cdr(args)) : NIL;

    /* Validate string argument */
    if (str_obj == NULL || str_obj == NIL) {
        return lisp_make_error("string->number: first argument cannot be nil");
    }
    if (str_obj->type != LISP_STRING) {
        return lisp_make_error("string->number: first argument must be a string");
    }

    const char *str = str_obj->value.string;
    int radix = 10; /* default base */

    /* Parse optional radix */
    if (radix_obj != NIL) {
        if (radix_obj->type != LISP_INTEGER) {
            return lisp_make_error("string->number: radix must be an integer");
        }
        radix = (int)radix_obj->value.integer;
        if (radix < 2 || radix > 36) {
            return lisp_make_error("string->number: radix must be between 2 and 36");
        }
    }

    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }

    /* Empty string returns #f */
    if (*str == '\0') {
        return NIL;
    }

    /* Handle radix prefix (only if radix not explicitly specified) */
    if (radix == 10 && str[0] == '#' && str[1]) {
        char prefix = str[1];
        if (prefix == 'b' || prefix == 'B') {
            radix = 2;
            str += 2;
        } else if (prefix == 'o' || prefix == 'O') {
            radix = 8;
            str += 2;
        } else if (prefix == 'd' || prefix == 'D') {
            radix = 10;
            str += 2;
        } else if (prefix == 'x' || prefix == 'X') {
            radix = 16;
            str += 2;
        }
    }

    /* Try parsing as float (only for base 10) */
    if (radix == 10 && (strchr(str, '.') != NULL || strchr(str, 'e') != NULL || strchr(str, 'E') != NULL)) {
        char *endptr;
        errno = 0;
        double value = strtod(str, &endptr);

        /* Skip trailing whitespace */
        while (*endptr && isspace((unsigned char)*endptr)) {
            endptr++;
        }

        /* Return float if parse succeeded, else #f */
        if (*endptr == '\0' && errno != ERANGE) {
            return lisp_make_number(value);
        }
        return NIL;
    }

    /* Try parsing as integer */
    char *endptr;
    errno = 0;
    long long value = strtoll(str, &endptr, radix);

    /* Skip trailing whitespace */
    while (*endptr && isspace((unsigned char)*endptr)) {
        endptr++;
    }

    /* Return integer if parse succeeded, else #f */
    if (*endptr != '\0' || errno == ERANGE) {
        return NIL;
    }

    return lisp_make_integer(value);
}

static LispObject *builtin_split(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("split");

    LispObject *str_obj = lisp_car(args);
    LispObject *pattern_obj = lisp_car(lisp_cdr(args));

    if (str_obj->type != LISP_STRING || pattern_obj->type != LISP_STRING) {
        return lisp_make_error("split requires strings");
    }

    const char *str = str_obj->value.string;
    const char *pattern = pattern_obj->value.string;
    size_t pattern_len = strlen(pattern);

    /* Handle empty pattern */
    if (pattern_len == 0) {
        return lisp_make_cons(lisp_make_string(str), NIL);
    }

    LispObject *result = NIL;
    LispObject *tail = NULL;

    const char *start = str;
    const char *p = str;

    /* Check if pattern contains wildcards */
    int has_wildcards = (strchr(pattern, '*') != NULL || strchr(pattern, '?') != NULL);

    while (*p) {
        int match = 0;

        if (has_wildcards) {
            match = wildcard_match(pattern, p);
        } else {
            /* Literal string match */
            match = (strncmp(p, pattern, pattern_len) == 0);
        }

        if (match) {
            /* Found match */
            size_t len = p - start;
            char *token = GC_malloc(len + 1);
            strncpy(token, start, len);
            token[len] = '\0';

            LIST_APPEND(result, tail, lisp_make_string(token));

            /* Skip pattern */
            p += pattern_len;
            start = p;
        } else {
            p++;
        }
    }

    /* Add remaining */
    if (*start || result != NIL)
        LIST_APPEND(result, tail, lisp_make_string(start));

    return result;
}

static LispObject *builtin_join(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("join");

    LispObject *list_obj = lisp_car(args);
    LispObject *sep_obj = lisp_car(lisp_cdr(args));

    if (sep_obj->type != LISP_STRING) {
        return lisp_make_error("join: separator must be a string");
    }

    const char *sep = sep_obj->value.string;
    size_t sep_len = strlen(sep);

    /* Handle empty list */
    if (list_obj == NIL) {
        return lisp_make_string("");
    }

    /* Calculate total length */
    size_t total_len = 0;
    size_t count = 0;
    LispObject *curr = list_obj;
    while (curr != NIL && curr != NULL && curr->type == LISP_CONS) {
        LispObject *elem = lisp_car(curr);
        if (elem->type != LISP_STRING) {
            return lisp_make_error("join: all list elements must be strings");
        }
        total_len += strlen(elem->value.string);
        count++;
        curr = lisp_cdr(curr);
    }

    /* Add separator lengths (count - 1 separators) */
    if (count > 1) {
        total_len += sep_len * (count - 1);
    }

    /* Build result string */
    char *result = GC_malloc(total_len + 1);
    result[0] = '\0';

    curr = list_obj;
    int first = 1;
    while (curr != NIL && curr != NULL && curr->type == LISP_CONS) {
        if (!first) {
            strcat(result, sep);
        }
        first = 0;
        LispObject *elem = lisp_car(curr);
        strcat(result, elem->value.string);
        curr = lisp_cdr(curr);
    }

    return lisp_make_string(result);
}

#define DEFINE_STR_CMP(cname, opname, op)                              \
    static LispObject *cname(LispObject *args, Environment *env)       \
    {                                                                  \
        (void)env;                                                     \
        CHECK_ARGS_2(opname);                                          \
        LispObject *a = lisp_car(args), *b = lisp_car(lisp_cdr(args)); \
        if (a->type != LISP_STRING || b->type != LISP_STRING)          \
            return lisp_make_error(opname " requires strings");        \
        return (strcmp(a->value.string, b->value.string) op 0)         \
                   ? LISP_TRUE                                         \
                   : NIL;                                              \
    }
DEFINE_STR_CMP(builtin_string_lt, "string<?", <)
DEFINE_STR_CMP(builtin_string_gt, "string>?", >)
DEFINE_STR_CMP(builtin_string_lte, "string<=?", <=)
DEFINE_STR_CMP(builtin_string_gte, "string>=?", >=)
#undef DEFINE_STR_CMP

static LispObject *builtin_string_contains_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("string-contains?");

    LispObject *haystack = lisp_car(args);
    LispObject *needle = lisp_car(lisp_cdr(args));

    if (haystack->type != LISP_STRING || needle->type != LISP_STRING) {
        return lisp_make_error("string-contains? requires strings");
    }

    return (strstr(haystack->value.string, needle->value.string) != NULL) ? LISP_TRUE : NIL;
}

static LispObject *builtin_string_index(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("string-index");

    LispObject *haystack = lisp_car(args);
    LispObject *needle = lisp_car(lisp_cdr(args));

    if (haystack->type != LISP_STRING || needle->type != LISP_STRING) {
        return lisp_make_error("string-index requires strings");
    }

    /* Find byte offset where needle occurs in haystack */
    char *found = strstr(haystack->value.string, needle->value.string);
    if (found == NULL) {
        return NIL;
    }

    /* Count UTF-8 characters from start to found position */
    int char_index = 0;
    const char *ptr = haystack->value.string;
    while (ptr < found) {
        ptr = utf8_next_char(ptr);
        if (ptr == NULL) {
            break;
        }
        char_index++;
    }

    return lisp_make_integer(char_index);
}

static LispObject *builtin_string_match_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("string-match?");

    LispObject *str = lisp_car(args);
    LispObject *pattern = lisp_car(lisp_cdr(args));

    if (str->type != LISP_STRING || pattern->type != LISP_STRING) {
        return lisp_make_error("string-match? requires strings");
    }

    return wildcard_match(pattern->value.string, str->value.string) ? LISP_TRUE : NIL;
}

static LispObject *builtin_string_prefix_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("string-prefix?");

    LispObject *prefix = lisp_car(args);
    LispObject *str = lisp_car(lisp_cdr(args));

    if (prefix->type != LISP_STRING || str->type != LISP_STRING) {
        return lisp_make_error("string-prefix? requires strings");
    }

    size_t prefix_len = strlen(prefix->value.string);
    size_t str_len = strlen(str->value.string);

    /* If prefix is longer than string, it can't be a prefix */
    if (prefix_len > str_len) {
        return NIL;
    }

    /* Use strncmp to check if prefix matches the beginning of str */
    return (strncmp(prefix->value.string, str->value.string, prefix_len) == 0) ? LISP_TRUE : NIL;
}

/* UTF-8 String operations */
static LispObject *builtin_substring(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL || lisp_cdr(args) == NIL) {
        return lisp_make_error("substring requires at least 2 arguments");
    }

    LispObject *str_obj = lisp_car(args);
    LispObject *start_obj = lisp_car(lisp_cdr(args));
    LispObject *end_obj = lisp_cdr(lisp_cdr(args)) != NIL ? lisp_car(lisp_cdr(lisp_cdr(args))) : NIL;

    if (str_obj->type != LISP_STRING) {
        return lisp_make_error("substring requires a string");
    }
    if (start_obj->type != LISP_INTEGER) {
        return lisp_make_error("substring requires integer start index");
    }

    long long start = start_obj->value.integer;
    long long end;

    if (end_obj == NIL || end_obj == NULL) {
        /* No end specified, use string length (codepoint count) */
        end = utf8_strlen(str_obj->value.string);
    } else {
        if (end_obj->type != LISP_INTEGER) {
            return lisp_make_error("substring requires integer end index");
        }
        end = end_obj->value.integer;
    }

    if (start < 0 || end < 0 || start > end) {
        return lisp_make_error("substring: invalid start/end indices");
    }

    /* Use codepoint-based indexing for consistency with length and string-ref */
    size_t start_offset = utf8_byte_offset(str_obj->value.string, start);
    size_t end_offset = utf8_byte_offset(str_obj->value.string, end);

    size_t result_len = end_offset - start_offset;
    char *result = GC_malloc(result_len + 1);
    memcpy(result, str_obj->value.string + start_offset, result_len);
    result[result_len] = '\0';

    return lisp_make_string(result);
}

static LispObject *builtin_string_ref(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("string-ref");

    LispObject *str_obj = lisp_car(args);
    LispObject *index_obj = lisp_car(lisp_cdr(args));

    if (str_obj->type != LISP_STRING) {
        return lisp_make_error("string-ref requires a string");
    }
    if (index_obj->type != LISP_INTEGER) {
        return lisp_make_error("string-ref requires an integer index");
    }

    long long index = index_obj->value.integer;
    if (index < 0) {
        return lisp_make_error("string-ref: negative index");
    }

    size_t char_count = utf8_strlen(str_obj->value.string);
    if (index >= (long long)char_count) {
        return lisp_make_error("string-ref: index out of bounds");
    }

    const char *char_ptr = utf8_char_at(str_obj->value.string, index);
    if (char_ptr == NULL) {
        return lisp_make_error("string-ref: invalid character at index");
    }

    unsigned int codepoint = utf8_get_codepoint(char_ptr);
    return lisp_make_char(codepoint);
}

/* String replace */
static LispObject *builtin_string_replace(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_3("string-replace");

    LispObject *str_obj = lisp_car(args);
    LispObject *old_obj = lisp_car(lisp_cdr(args));
    LispObject *new_obj = lisp_car(lisp_cdr(lisp_cdr(args)));

    if (old_obj->type != LISP_STRING || new_obj->type != LISP_STRING || str_obj->type != LISP_STRING) {
        return lisp_make_error("string-replace requires strings");
    }

    const char *old_str = old_obj->value.string;
    const char *new_str = new_obj->value.string;
    const char *str = str_obj->value.string;

    /* If old string is empty, return original string */
    if (old_str[0] == '\0') {
        return lisp_make_string(GC_strdup(str));
    }

    /* Count occurrences to estimate result size */
    size_t old_len = strlen(old_str);
    size_t new_len = strlen(new_str);
    size_t str_len = strlen(str);
    size_t count = 0;
    const char *pos = str;
    while ((pos = strstr(pos, old_str)) != NULL) {
        count++;
        pos += old_len;
    }

    /* Calculate result size */
    size_t result_len = str_len + (new_len - old_len) * count;
    char *result = GC_malloc(result_len + 1);
    char *result_ptr = result;
    const char *str_ptr = str;

    /* Replace all occurrences */
    while ((pos = strstr(str_ptr, old_str)) != NULL) {
        /* Copy part before match */
        size_t before_len = pos - str_ptr;
        memcpy(result_ptr, str_ptr, before_len);
        result_ptr += before_len;

        /* Copy new string */
        memcpy(result_ptr, new_str, new_len);
        result_ptr += new_len;

        /* Advance past old string */
        str_ptr = pos + old_len;
    }

    /* Copy remaining part */
    size_t remaining = strlen(str_ptr);
    memcpy(result_ptr, str_ptr, remaining);
    result_ptr += remaining;
    *result_ptr = '\0';

    return lisp_make_string(result);
}

static LispObject *string_case_convert(const char *str,
                                       int (*convert)(int))
{
    size_t len = strlen(str);
    char *result = GC_malloc(len + 1);
    const char *src = str;
    char *dst = result;

    while (*src) {
        int codepoint = utf8_get_codepoint(src);
        if (codepoint >= 0 && codepoint < 128) {
            *dst = convert((unsigned char)*src);
            src++;
            dst++;
        } else {
            int bytes = utf8_char_bytes(src);
            memcpy(dst, src, bytes);
            src += bytes;
            dst += bytes;
        }
    }
    *dst = '\0';
    return lisp_make_string(result);
}

static LispObject *builtin_string_upcase(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("string-upcase");
    LispObject *str_obj = lisp_car(args);
    if (str_obj->type != LISP_STRING)
        return lisp_make_error("string-upcase requires a string");
    return string_case_convert(str_obj->value.string, toupper);
}

static LispObject *builtin_string_downcase(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("string-downcase");
    LispObject *str_obj = lisp_car(args);
    if (str_obj->type != LISP_STRING)
        return lisp_make_error("string-downcase requires a string");
    return string_case_convert(str_obj->value.string, tolower);
}

/* String trim - remove leading and trailing whitespace */
static LispObject *builtin_string_trim(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("string-trim");

    LispObject *str_obj = lisp_car(args);
    if (str_obj->type != LISP_STRING) {
        return lisp_make_error("string-trim requires a string");
    }

    const char *str = str_obj->value.string;
    size_t len = strlen(str);

    /* Find start - skip leading whitespace */
    const char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    /* If all whitespace, return empty string */
    if (*start == '\0') {
        return lisp_make_string("");
    }

    /* Find end - skip trailing whitespace */
    const char *end = str + len - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }

    /* Calculate result length and copy */
    size_t result_len = end - start + 1;
    char *result = GC_malloc(result_len + 1);
    memcpy(result, start, result_len);
    result[result_len] = '\0';

    return lisp_make_string(result);
}

void register_strings_builtins(Environment *env)
{
    REGISTER("concat", builtin_concat);
    /* Note: string-append is defined as an alias via defalias in stdlib */
    REGISTER("number->string", builtin_number_to_string);
    REGISTER("string->number", builtin_string_to_number);
    REGISTER("split", builtin_split);
    REGISTER("join", builtin_join);
    REGISTER("string<?", builtin_string_lt);
    REGISTER("string>?", builtin_string_gt);
    REGISTER("string<=?", builtin_string_lte);
    REGISTER("string>=?", builtin_string_gte);
    REGISTER("string-contains?", builtin_string_contains_question);
    REGISTER("string-index", builtin_string_index);
    REGISTER("string-match?", builtin_string_match_question);
    REGISTER("substring", builtin_substring);
    REGISTER("string-ref", builtin_string_ref);
    REGISTER("string-prefix?", builtin_string_prefix_question);
    REGISTER("string-replace", builtin_string_replace);
    REGISTER("string-upcase", builtin_string_upcase);
    REGISTER("string-downcase", builtin_string_downcase);
    REGISTER("string-trim", builtin_string_trim);
}

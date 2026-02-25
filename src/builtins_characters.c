#include "builtins_internal.h"

static LispObject *builtin_char_question(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return NIL;
    }
    LispObject *obj = lisp_car(args);
    return lisp_make_boolean(obj->type == LISP_CHAR);
}

static LispObject *builtin_char_code(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("char-code");
    LispObject *char_obj = lisp_car(args);
    if (char_obj->type != LISP_CHAR) {
        return lisp_make_error("char-code requires a character");
    }
    return lisp_make_integer(char_obj->value.character);
}

static LispObject *builtin_code_char(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("code-char");
    LispObject *code_obj = lisp_car(args);
    if (code_obj->type != LISP_INTEGER) {
        return lisp_make_error("code-char requires an integer");
    }
    long long code = code_obj->value.integer;
    if (code < 0 || code > 0x10FFFF) {
        return lisp_make_error("code-char: invalid Unicode codepoint");
    }
    return lisp_make_char((unsigned int)code);
}

static LispObject *builtin_char_to_string(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("char->string");
    LispObject *char_obj = lisp_car(args);
    if (char_obj->type != LISP_CHAR) {
        return lisp_make_error("char->string requires a character");
    }
    char buf[5];
    utf8_put_codepoint(char_obj->value.character, buf);
    return lisp_make_string(buf);
}

static LispObject *builtin_string_to_char(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("string->char");
    LispObject *str_obj = lisp_car(args);
    if (str_obj->type != LISP_STRING) {
        return lisp_make_error("string->char requires a string");
    }
    const char *str = str_obj->value.string;
    size_t char_count = utf8_strlen(str);
    if (char_count != 1) {
        return lisp_make_error("string->char requires a single-character string");
    }
    unsigned int codepoint = utf8_get_codepoint(str);
    return lisp_make_char(codepoint);
}

#define DEFINE_CHAR_CMP(cname, opname, op)                                    \
    static LispObject *cname(LispObject *args, Environment *env)              \
    {                                                                         \
        (void)env;                                                            \
        CHECK_ARGS_2(opname);                                                 \
        LispObject *c1 = lisp_car(args), *c2 = lisp_car(lisp_cdr(args));      \
        if (c1->type != LISP_CHAR || c2->type != LISP_CHAR)                   \
            return lisp_make_error(opname " requires characters");            \
        return lisp_make_boolean(c1->value.character op c2->value.character); \
    }
DEFINE_CHAR_CMP(builtin_char_eq, "char=?", ==)
DEFINE_CHAR_CMP(builtin_char_lt, "char<?", <)
DEFINE_CHAR_CMP(builtin_char_gt, "char>?", >)
DEFINE_CHAR_CMP(builtin_char_lte, "char<=?", <=)
DEFINE_CHAR_CMP(builtin_char_gte, "char>=?", >=)
#undef DEFINE_CHAR_CMP

static LispObject *builtin_char_upcase(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("char-upcase");
    LispObject *char_obj = lisp_car(args);
    if (char_obj->type != LISP_CHAR) {
        return lisp_make_error("char-upcase requires a character");
    }
    unsigned int cp = char_obj->value.character;
    /* ASCII only case conversion */
    if (cp >= 'a' && cp <= 'z') {
        cp = cp - 'a' + 'A';
    }
    return lisp_make_char(cp);
}

static LispObject *builtin_char_downcase(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("char-downcase");
    LispObject *char_obj = lisp_car(args);
    if (char_obj->type != LISP_CHAR) {
        return lisp_make_error("char-downcase requires a character");
    }
    unsigned int cp = char_obj->value.character;
    /* ASCII only case conversion */
    if (cp >= 'A' && cp <= 'Z') {
        cp = cp - 'A' + 'a';
    }
    return lisp_make_char(cp);
}

static LispObject *builtin_char_alphabetic_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("char-alphabetic?");
    LispObject *char_obj = lisp_car(args);
    if (char_obj->type != LISP_CHAR) {
        return lisp_make_error("char-alphabetic? requires a character");
    }
    unsigned int cp = char_obj->value.character;
    /* ASCII only check */
    int is_alpha = (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z');
    return lisp_make_boolean(is_alpha);
}

static LispObject *builtin_char_numeric_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("char-numeric?");
    LispObject *char_obj = lisp_car(args);
    if (char_obj->type != LISP_CHAR) {
        return lisp_make_error("char-numeric? requires a character");
    }
    unsigned int cp = char_obj->value.character;
    int is_numeric = (cp >= '0' && cp <= '9');
    return lisp_make_boolean(is_numeric);
}

static LispObject *builtin_char_whitespace_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("char-whitespace?");
    LispObject *char_obj = lisp_car(args);
    if (char_obj->type != LISP_CHAR) {
        return lisp_make_error("char-whitespace? requires a character");
    }
    unsigned int cp = char_obj->value.character;
    int is_ws = (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == '\f' || cp == '\v');
    return lisp_make_boolean(is_ws);
}

void register_characters_builtins(Environment *env)
{
    REGISTER("char?", builtin_char_question);
    REGISTER("char-code", builtin_char_code);
    REGISTER("code-char", builtin_code_char);
    REGISTER("char->string", builtin_char_to_string);
    REGISTER("string->char", builtin_string_to_char);
    REGISTER("char=?", builtin_char_eq);
    REGISTER("char<?", builtin_char_lt);
    REGISTER("char>?", builtin_char_gt);
    REGISTER("char<=?", builtin_char_lte);
    REGISTER("char>=?", builtin_char_gte);
    REGISTER("char-upcase", builtin_char_upcase);
    REGISTER("char-downcase", builtin_char_downcase);
    REGISTER("char-alphabetic?", builtin_char_alphabetic_question);
    REGISTER("char-numeric?", builtin_char_numeric_question);
    REGISTER("char-whitespace?", builtin_char_whitespace_question);
}

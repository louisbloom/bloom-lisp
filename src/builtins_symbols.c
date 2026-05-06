#include "builtins_internal.h"

static LispObject *builtin_symbol_to_string(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("symbol->string");
    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_SYMBOL) {
        return lisp_make_error("symbol->string requires a symbol argument");
    }
    return lisp_make_string(LISP_SYM_VAL(arg)->name);
}

static LispObject *builtin_string_to_symbol(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("string->symbol");
    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_STRING) {
        return lisp_make_error("string->symbol requires a string argument");
    }
    return lisp_intern(LISP_STR_VAL(arg));
}

static LispObject *builtin_keyword_name(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("keyword-name");
    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_KEYWORD) {
        return lisp_make_error("keyword-name requires a keyword argument");
    }
    /* Return name without the leading colon */
    const char *name = LISP_SYM_VAL(arg)->name;
    if (name[0] == ':') {
        return lisp_make_string(name + 1);
    }
    return lisp_make_string(name);
}

void register_symbols_builtins(Environment *env)
{
    REGISTER("symbol->string", builtin_symbol_to_string);
    REGISTER("string->symbol", builtin_string_to_symbol);
    REGISTER("keyword-name", builtin_keyword_name);
}

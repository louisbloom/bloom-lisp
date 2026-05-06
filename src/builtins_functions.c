#include "builtins_internal.h"

static LispObject *builtin_function_params(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("function-params");
    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) == LISP_LAMBDA) {
        return LISP_LAMBDA_PARAMS(arg) ? LISP_LAMBDA_PARAMS(arg) : NIL;
    }
    if (LISP_TYPE(arg) == LISP_MACRO) {
        return LISP_MACRO_PARAMS(arg) ? LISP_MACRO_PARAMS(arg) : NIL;
    }
    return lisp_make_error("function-params requires a lambda or macro");
}

static LispObject *builtin_function_body(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("function-body");
    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) == LISP_LAMBDA) {
        return LISP_LAMBDA_BODY(arg) ? LISP_LAMBDA_BODY(arg) : NIL;
    }
    if (LISP_TYPE(arg) == LISP_MACRO) {
        return LISP_MACRO_BODY(arg) ? LISP_MACRO_BODY(arg) : NIL;
    }
    return lisp_make_error("function-body requires a lambda or macro");
}

static LispObject *builtin_function_name(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("function-name");
    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) == LISP_LAMBDA) {
        return LISP_LAMBDA_NAME(arg) ? lisp_make_string(LISP_LAMBDA_NAME(arg)) : NIL;
    }
    if (LISP_TYPE(arg) == LISP_MACRO) {
        return LISP_MACRO_NAME(arg) ? lisp_make_string(LISP_MACRO_NAME(arg)) : NIL;
    }
    if (LISP_TYPE(arg) == LISP_BUILTIN) {
        return LISP_BUILTIN_NAME(arg) ? lisp_make_string(LISP_BUILTIN_NAME(arg)) : NIL;
    }
    return lisp_make_error("function-name requires a lambda, macro, or builtin");
}

static LispObject *builtin_documentation(LispObject *args, Environment *env)
{
    (void)env; /* Docstrings are on symbols, not looked up in env */

    if (args == NIL) {
        return lisp_make_error("documentation requires at least 1 argument");
    }

    LispObject *symbol = lisp_car(args);

    if (LISP_TYPE(symbol) != LISP_SYMBOL) {
        return lisp_make_error("documentation requires a symbol");
    }

    /* Docstrings are stored on the symbol (set by REGISTER, define, or set-documentation!) */
    if (LISP_SYM_VAL(symbol)->docstring != NULL) {
        return lisp_make_string(LISP_SYM_VAL(symbol)->docstring);
    }

    return NIL;
}

static LispObject *builtin_bound_question(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("bound?");

    LispObject *symbol = lisp_car(args);

    if (LISP_TYPE(symbol) != LISP_SYMBOL) {
        return lisp_make_error("bound? requires a symbol");
    }

    /* Check if the symbol is bound */
    LispObject *value = env_lookup(env, LISP_SYM_VAL(symbol));

    return value != NULL ? LISP_TRUE : NIL;
}

static LispObject *builtin_set_documentation_bang(LispObject *args, Environment *env)
{
    (void)env; /* Unused - docstrings are stored on symbols, not bindings */

    CHECK_ARGS_2("set-documentation!");

    LispObject *symbol = lisp_car(args);
    LispObject *docstring = lisp_car(lisp_cdr(args));

    if (LISP_TYPE(symbol) != LISP_SYMBOL) {
        return lisp_make_error("set-documentation! first argument must be a symbol");
    }

    if (LISP_TYPE(docstring) != LISP_STRING) {
        return lisp_make_error("set-documentation! second argument must be a string");
    }

    /* Set the docstring directly on the interned symbol */
    LISP_SYM_VAL(symbol)->docstring = GC_strdup(LISP_STR_VAL(docstring));

    return LISP_TRUE;
}

static LispObject *builtin_eval(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("eval");

    LispObject *expr = lisp_car(args);

    /* Evaluate the expression in the current environment */
    return lisp_eval(expr, env);
}

static LispObject *builtin_exit(LispObject *args, Environment *env)
{
    (void)env;
    int code = 0;
    if (args != NIL) {
        LispObject *arg = lisp_car(args);
        if (LISP_TYPE(arg) == LISP_INTEGER) {
            code = (int)LISP_INT_VAL(arg);
        } else if (LISP_TYPE(arg) == LISP_NUMBER) {
            code = (int)LISP_NUM_VAL(arg);
        } else {
            return lisp_make_error("exit: argument must be a number");
        }
    }
    exit(code);
    return NIL; /* unreachable */
}

void register_functions_builtins(Environment *env)
{
    REGISTER("function-params", builtin_function_params);
    REGISTER("function-body", builtin_function_body);
    REGISTER("function-name", builtin_function_name);
    REGISTER("documentation", builtin_documentation);
    REGISTER("bound?", builtin_bound_question);
    REGISTER("set-documentation!", builtin_set_documentation_bang);
    REGISTER("eval", builtin_eval);
    REGISTER("exit", builtin_exit);
    REGISTER("quit", builtin_exit);
}

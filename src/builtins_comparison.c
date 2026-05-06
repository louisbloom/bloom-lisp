#include "builtins_internal.h"

/* Number comparisons */
#define DEFINE_NUM_CMP(cname, opname, op)                              \
    static LispObject *cname(LispObject *args, Environment *env)       \
    {                                                                  \
        (void)env;                                                     \
        CHECK_ARGS_2(opname);                                          \
        LispObject *a = lisp_car(args), *b = lisp_car(lisp_cdr(args)); \
        int ai, bi;                                                    \
        double av = get_numeric_value(a, &ai),                         \
               bv = get_numeric_value(b, &bi);                         \
        if ((!ai && LISP_TYPE(a) != LISP_NUMBER) ||                    \
            (!bi && LISP_TYPE(b) != LISP_NUMBER))                      \
            return lisp_make_error(opname " requires numbers");        \
        return (av op bv) ? LISP_TRUE : NIL;                           \
    }
DEFINE_NUM_CMP(builtin_gt, ">", >)
DEFINE_NUM_CMP(builtin_lt, "<", <)
DEFINE_NUM_CMP(builtin_eq, "=", ==)
DEFINE_NUM_CMP(builtin_gte, ">=", >=)
DEFINE_NUM_CMP(builtin_lte, "<=", <=)
#undef DEFINE_NUM_CMP

/* Boolean operations */
static LispObject *builtin_not(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("not");

    LispObject *arg = lisp_car(args);
    return lisp_is_truthy(arg) ? NIL : LISP_TRUE;
}

/* Equality predicates */

static LispObject *builtin_eq_question(LispObject *args, Environment *env)
{
    (void)env;
    /* Validate exactly 2 arguments */
    if (args == NIL || LISP_CDR(args) == NIL || LISP_CDR(LISP_CDR(args)) != NIL) {
        return lisp_make_error("eq? expects exactly 2 arguments");
    }

    LispObject *a = LISP_CAR(args);
    LispObject *b = LISP_CAR(LISP_CDR(args));

    /* Pointer equality - same object in memory */
    return (a == b) ? LISP_TRUE : NIL;
}

static LispObject *builtin_equal_question(LispObject *args, Environment *env)
{
    (void)env;
    /* Validate exactly 2 arguments */
    if (args == NIL || LISP_CDR(args) == NIL || LISP_CDR(LISP_CDR(args)) != NIL) {
        return lisp_make_error("equal? expects exactly 2 arguments");
    }

    LispObject *a = LISP_CAR(args);
    LispObject *b = LISP_CAR(LISP_CDR(args));

    /* Use recursive structural equality */
    return objects_equal_recursive(a, b) ? LISP_TRUE : NIL;
}

static LispObject *builtin_string_eq_question(LispObject *args, Environment *env)
{
    (void)env;
    /* Validate exactly 2 arguments */
    if (args == NIL || LISP_CDR(args) == NIL || LISP_CDR(LISP_CDR(args)) != NIL) {
        return lisp_make_error("string=? expects exactly 2 arguments");
    }

    LispObject *a = LISP_CAR(args);
    LispObject *b = LISP_CAR(LISP_CDR(args));

    /* Type validation */
    if (LISP_TYPE(a) != LISP_STRING) {
        return lisp_make_error("string=?: first argument must be a string");
    }
    if (LISP_TYPE(b) != LISP_STRING) {
        return lisp_make_error("string=?: second argument must be a string");
    }

    /* String comparison */
    return (strcmp(LISP_STR_VAL(a), LISP_STR_VAL(b)) == 0) ? LISP_TRUE : NIL;
}

void register_comparison_builtins(Environment *env)
{
    REGISTER(">", builtin_gt);
    REGISTER("<", builtin_lt);
    REGISTER("=", builtin_eq);
    REGISTER(">=", builtin_gte);
    REGISTER("<=", builtin_lte);
    REGISTER("not", builtin_not);
    REGISTER("eq?", builtin_eq_question);
    REGISTER("equal?", builtin_equal_question);
    REGISTER("string=?", builtin_string_eq_question);
}

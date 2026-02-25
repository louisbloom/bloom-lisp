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
        if ((!ai && a->type != LISP_NUMBER) ||                         \
            (!bi && b->type != LISP_NUMBER))                           \
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
    if (args == NIL || args->value.cons.cdr == NIL || args->value.cons.cdr->value.cons.cdr != NIL) {
        return lisp_make_error("eq? expects exactly 2 arguments");
    }

    LispObject *a = args->value.cons.car;
    LispObject *b = args->value.cons.cdr->value.cons.car;

    /* Pointer equality - same object in memory */
    return (a == b) ? LISP_TRUE : NIL;
}

static LispObject *builtin_equal_question(LispObject *args, Environment *env)
{
    (void)env;
    /* Validate exactly 2 arguments */
    if (args == NIL || args->value.cons.cdr == NIL || args->value.cons.cdr->value.cons.cdr != NIL) {
        return lisp_make_error("equal? expects exactly 2 arguments");
    }

    LispObject *a = args->value.cons.car;
    LispObject *b = args->value.cons.cdr->value.cons.car;

    /* Use recursive structural equality */
    return objects_equal_recursive(a, b) ? LISP_TRUE : NIL;
}

static LispObject *builtin_string_eq_question(LispObject *args, Environment *env)
{
    (void)env;
    /* Validate exactly 2 arguments */
    if (args == NIL || args->value.cons.cdr == NIL || args->value.cons.cdr->value.cons.cdr != NIL) {
        return lisp_make_error("string=? expects exactly 2 arguments");
    }

    LispObject *a = args->value.cons.car;
    LispObject *b = args->value.cons.cdr->value.cons.car;

    /* Type validation */
    if (a->type != LISP_STRING) {
        return lisp_make_error("string=?: first argument must be a string");
    }
    if (b->type != LISP_STRING) {
        return lisp_make_error("string=?: second argument must be a string");
    }

    /* String comparison */
    return (strcmp(a->value.string, b->value.string) == 0) ? LISP_TRUE : NIL;
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

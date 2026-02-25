#include "builtins_internal.h"

/* Arithmetic loop: validate + accumulate numeric args */
#define ARITH_LOOP(accum, op, name)                                      \
    while (args != NIL && args != NULL) {                                \
        LispObject *_a = lisp_car(args);                                 \
        int _ai = 0;                                                     \
        double _v = get_numeric_value(_a, &_ai);                         \
        if (!_ai && _a->type != LISP_NUMBER && _a->type != LISP_INTEGER) \
            return lisp_make_error(name " requires numbers");            \
        if (!_ai)                                                        \
            all_integers = 0;                                            \
        accum op _v;                                                     \
        args = lisp_cdr(args);                                           \
    }

/* Return integer if all args were ints, else float */
#define ARITH_RETURN(accum)                                     \
    return all_integers ? lisp_make_integer((long long)(accum)) \
                        : lisp_make_number(accum)

static LispObject *builtin_add(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_integer(0);

    int all_integers = 1;
    double sum = 0;
    ARITH_LOOP(sum, +=, "+");
    ARITH_RETURN(sum);
}

static LispObject *builtin_subtract(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("-");

    LispObject *first = lisp_car(args);
    int first_is_integer = 0;
    double result = get_numeric_value(first, &first_is_integer);
    int all_integers = first_is_integer;
    args = lisp_cdr(args);

    if (args == NIL)
        return first_is_integer ? lisp_make_integer((long long)-result)
                                : lisp_make_number(-result);

    ARITH_LOOP(result, -=, "-");
    ARITH_RETURN(result);
}

static LispObject *builtin_multiply(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_integer(1);

    int all_integers = 1;
    double product = 1;
    ARITH_LOOP(product, *=, "*");
    ARITH_RETURN(product);
}
#undef ARITH_LOOP
#undef ARITH_RETURN

static LispObject *builtin_divide(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_error("/ requires at least one argument");
    }

    LispObject *first = lisp_car(args);
    int first_is_integer;
    double result = get_numeric_value(first, &first_is_integer);
    args = lisp_cdr(args);

    if (args == NIL) {
        /* Unary reciprocal */
        if (result == 0) {
            return lisp_make_error("Division by zero");
        }
        return lisp_make_number(1.0 / result); /* Always return float */
    }

    /* Division always returns float */
    while (args != NIL && args != NULL) {
        LispObject *arg = lisp_car(args);
        int arg_is_integer = 0;
        double val = get_numeric_value(arg, &arg_is_integer);
        if (arg_is_integer == 0 && arg->type != LISP_NUMBER && arg->type != LISP_INTEGER) {
            return lisp_make_error("/ requires numbers");
        }
        if (val == 0) {
            return lisp_make_error("Division by zero");
        }
        result /= val;
        args = lisp_cdr(args);
    }

    return lisp_make_number(result); /* Always return float */
}

static LispObject *builtin_quotient(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("quotient");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (first->type != LISP_INTEGER && first->type != LISP_NUMBER) {
        return lisp_make_error("quotient requires numbers");
    }
    if (second->type != LISP_INTEGER && second->type != LISP_NUMBER) {
        return lisp_make_error("quotient requires numbers");
    }

    int first_is_integer;
    int second_is_integer;
    double first_val = get_numeric_value(first, &first_is_integer);
    double second_val = get_numeric_value(second, &second_is_integer);

    if (second_val == 0) {
        return lisp_make_error("Division by zero");
    }

    /* Truncate to integer */
    long long result = (long long)(first_val / second_val);
    return lisp_make_integer(result);
}

static LispObject *builtin_remainder(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("remainder");

    LispObject *first = lisp_car(args);
    LispObject *second = lisp_car(lisp_cdr(args));

    if (first->type != LISP_INTEGER && first->type != LISP_NUMBER) {
        return lisp_make_error("remainder requires numbers");
    }
    if (second->type != LISP_INTEGER && second->type != LISP_NUMBER) {
        return lisp_make_error("remainder requires numbers");
    }

    int first_is_integer;
    int second_is_integer;
    double first_val = get_numeric_value(first, &first_is_integer);
    double second_val = get_numeric_value(second, &second_is_integer);

    if (second_val == 0) {
        return lisp_make_error("Division by zero");
    }

    long long result = (long long)first_val % (long long)second_val;
    return lisp_make_integer(result);
}

static LispObject *builtin_even_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("even?");

    LispObject *arg = lisp_car(args);
    int arg_is_integer;
    double arg_val = get_numeric_value(arg, &arg_is_integer);

    if (arg->type != LISP_INTEGER && arg->type != LISP_NUMBER) {
        return lisp_make_error("even? requires a number");
    }

    long long val = (long long)arg_val;
    if ((val & 1) == 0) {
        return lisp_make_boolean(1);
    }
    return lisp_make_boolean(0);
}

static LispObject *builtin_odd_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("odd?");

    LispObject *arg = lisp_car(args);
    int arg_is_integer;
    double arg_val = get_numeric_value(arg, &arg_is_integer);

    if (arg->type != LISP_INTEGER && arg->type != LISP_NUMBER) {
        return lisp_make_error("odd? requires a number");
    }

    long long val = (long long)arg_val;
    if ((val & 1) == 1) {
        return lisp_make_boolean(1);
    }
    return lisp_make_boolean(0);
}

void register_arithmetic_builtins(Environment *env)
{
    REGISTER("+", builtin_add);
    REGISTER("-", builtin_subtract);
    REGISTER("*", builtin_multiply);
    REGISTER("/", builtin_divide);
    REGISTER("quotient", builtin_quotient);
    REGISTER("remainder", builtin_remainder);
    REGISTER("even?", builtin_even_question);
    REGISTER("odd?", builtin_odd_question);
}

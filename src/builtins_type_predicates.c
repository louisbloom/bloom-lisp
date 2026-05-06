#include "builtins_internal.h"

static LispObject *builtin_null_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("null?");
    LispObject *arg = lisp_car(args);
    return (arg == NIL || arg == NULL) ? LISP_TRUE : NIL;
}

static LispObject *builtin_atom_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("atom?");
    LispObject *arg = lisp_car(args);
    return (LISP_TYPE(arg) != LISP_CONS) ? LISP_TRUE : NIL;
}

static LispObject *builtin_pair_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("pair?");
    LispObject *arg = lisp_car(args);
    return (LISP_TYPE(arg) == LISP_CONS) ? LISP_TRUE : NIL;
}

#define DEFINE_TYPE_PRED(cname, opname, check)                   \
    static LispObject *cname(LispObject *args, Environment *env) \
    {                                                            \
        (void)env;                                               \
        CHECK_ARGS_1(opname);                                    \
        LispObject *arg = lisp_car(args);                        \
        return (check) ? LISP_TRUE : NIL;                        \
    }
DEFINE_TYPE_PRED(builtin_integer_question, "integer?", LISP_TYPE(arg) == LISP_INTEGER)
DEFINE_TYPE_PRED(builtin_boolean_question, "boolean?", LISP_TYPE(arg) == LISP_BOOLEAN)
DEFINE_TYPE_PRED(builtin_number_question, "number?",
                 LISP_TYPE(arg) == LISP_NUMBER || LISP_TYPE(arg) == LISP_INTEGER)
DEFINE_TYPE_PRED(builtin_vector_question, "vector?", LISP_TYPE(arg) == LISP_VECTOR)
DEFINE_TYPE_PRED(builtin_hash_table_question, "hash-table?",
                 LISP_TYPE(arg) == LISP_HASH_TABLE)
DEFINE_TYPE_PRED(builtin_string_question, "string?", LISP_TYPE(arg) == LISP_STRING)
DEFINE_TYPE_PRED(builtin_symbol_question, "symbol?", LISP_TYPE(arg) == LISP_SYMBOL)
DEFINE_TYPE_PRED(builtin_list_question, "list?",
                 arg == NIL || LISP_TYPE(arg) == LISP_CONS)
DEFINE_TYPE_PRED(builtin_keyword_question, "keyword?",
                 LISP_TYPE(arg) == LISP_KEYWORD)
DEFINE_TYPE_PRED(builtin_function_question, "function?",
                 LISP_TYPE(arg) == LISP_LAMBDA)
DEFINE_TYPE_PRED(builtin_macro_question, "macro?", LISP_TYPE(arg) == LISP_MACRO)
DEFINE_TYPE_PRED(builtin_builtin_question, "builtin?",
                 LISP_TYPE(arg) == LISP_BUILTIN)
DEFINE_TYPE_PRED(builtin_callable_question, "callable?",
                 LISP_TYPE(arg) == LISP_LAMBDA || LISP_TYPE(arg) == LISP_MACRO ||
                     LISP_TYPE(arg) == LISP_BUILTIN)
DEFINE_TYPE_PRED(builtin_regex_question, "regex?", LISP_TYPE(arg) == LISP_REGEX)
#undef DEFINE_TYPE_PRED

void register_type_predicates_builtins(Environment *env)
{
    REGISTER("null?", builtin_null_question);
    REGISTER("atom?", builtin_atom_question);
    REGISTER("pair?", builtin_pair_question);
    REGISTER("integer?", builtin_integer_question);
    REGISTER("boolean?", builtin_boolean_question);
    REGISTER("number?", builtin_number_question);
    REGISTER("vector?", builtin_vector_question);
    REGISTER("hash-table?", builtin_hash_table_question);
    REGISTER("string?", builtin_string_question);
    REGISTER("symbol?", builtin_symbol_question);
    REGISTER("keyword?", builtin_keyword_question);
    REGISTER("list?", builtin_list_question);
    REGISTER("function?", builtin_function_question);
    REGISTER("macro?", builtin_macro_question);
    REGISTER("builtin?", builtin_builtin_question);
    REGISTER("callable?", builtin_callable_question);
    REGISTER("regex?", builtin_regex_question);
}

#ifndef BUILTINS_INTERNAL_H
#define BUILTINS_INTERNAL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../include/file_utils.h"
#include "../include/lisp.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "docstrings.gen.h"

/* ---- Argument validation macros ---- */
#define CHECK_ARGS_1(name) \
    if (args == NIL)       \
    return lisp_make_error(name " requires 1 argument")
#define CHECK_ARGS_2(name)                    \
    if (args == NIL || lisp_cdr(args) == NIL) \
    return lisp_make_error(name " requires 2 arguments")
#define CHECK_ARGS_3(name)                      \
    if (args == NIL || lisp_cdr(args) == NIL || \
        lisp_cdr(lisp_cdr(args)) == NIL)        \
    return lisp_make_error(name " requires 3 arguments")

/* ---- List builder macro ---- */
#define LIST_APPEND(result, tail, item)                  \
    do {                                                 \
        LispObject *_node = lisp_make_cons((item), NIL); \
        if ((result) == NIL) {                           \
            (result) = _node;                            \
            (tail) = _node;                              \
        } else {                                         \
            (tail)->value.cons.cdr = _node;              \
            (tail) = _node;                              \
        }                                                \
    } while (0)

/* Helper macro to register a builtin and set its symbol's docstring */
#define REGISTER(name, func)                                                         \
    do {                                                                             \
        LispObject *sym = lisp_intern(name);                                         \
        const char *_doc = lookup_builtin_doc(name);                                 \
        if (_doc != NULL)                                                            \
            sym->value.symbol->docstring = (char *)_doc;                             \
        env_define(env, sym->value.symbol, lisp_make_builtin(func, name), pkg_core); \
    } while (0)

/* Helper function to get numeric value */
static inline double get_numeric_value(LispObject *obj, int *is_integer)
{
    if (obj->type == LISP_INTEGER) {
        *is_integer = 1;
        return (double)obj->value.integer;
    } else if (obj->type == LISP_NUMBER) {
        *is_integer = 0;
        return obj->value.number;
    }
    return 0.0;
}

/* Cross-file dependency: defined in builtins_lists.c, used by builtins_comparison.c */
int objects_equal_recursive(LispObject *a, LispObject *b);

/* Per-topic registration functions */
void register_arithmetic_builtins(Environment *env);
void register_comparison_builtins(Environment *env);
void register_strings_builtins(Environment *env);
void register_characters_builtins(Environment *env);
void register_lists_builtins(Environment *env);
void register_vectors_builtins(Environment *env);
void register_hash_tables_builtins(Environment *env);
void register_regex_builtins(Environment *env);
void register_file_io_builtins(Environment *env);
void register_string_ports_builtins(Environment *env);
void register_type_predicates_builtins(Environment *env);
void register_symbols_builtins(Environment *env);
void register_functions_builtins(Environment *env);
void register_printing_builtins(Environment *env);
void register_errors_builtins(Environment *env);
void register_packages_builtins(Environment *env);
void register_filesystem_builtins(Environment *env);
void register_time_profiling_builtins(Environment *env);

#endif /* BUILTINS_INTERNAL_H */

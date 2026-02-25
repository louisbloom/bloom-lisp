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
#ifdef _WIN32
#include <windows.h>
#endif

/* Version information - fallbacks if not defined by autoconf */
#ifndef BLOOM_LISP_VERSION
#define BLOOM_LISP_VERSION "unknown"
#endif

#ifndef BLOOM_LISP_VERSION_MAJOR
#define BLOOM_LISP_VERSION_MAJOR 0
#endif

#ifndef BLOOM_LISP_VERSION_MINOR
#define BLOOM_LISP_VERSION_MINOR 0
#endif

#ifndef BLOOM_LISP_VERSION_PATCH
#define BLOOM_LISP_VERSION_PATCH 0
#endif

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

/* Arithmetic operations */
static LispObject *builtin_add(LispObject *args, Environment *env);
static LispObject *builtin_subtract(LispObject *args, Environment *env);
static LispObject *builtin_multiply(LispObject *args, Environment *env);
static LispObject *builtin_divide(LispObject *args, Environment *env);

/* Number comparisons */
static LispObject *builtin_gt(LispObject *args, Environment *env);
static LispObject *builtin_lt(LispObject *args, Environment *env);
static LispObject *builtin_eq(LispObject *args, Environment *env);
static LispObject *builtin_gte(LispObject *args, Environment *env);
static LispObject *builtin_lte(LispObject *args, Environment *env);

/* String operations */
static LispObject *builtin_concat(LispObject *args, Environment *env);
static LispObject *builtin_number_to_string(LispObject *args, Environment *env);
static LispObject *builtin_string_to_number(LispObject *args, Environment *env);
static LispObject *builtin_split(LispObject *args, Environment *env);
static LispObject *builtin_join(LispObject *args, Environment *env);
static LispObject *builtin_string_lt(LispObject *args, Environment *env);
static LispObject *builtin_string_gt(LispObject *args, Environment *env);
static LispObject *builtin_string_lte(LispObject *args, Environment *env);
static LispObject *builtin_string_gte(LispObject *args, Environment *env);
static LispObject *builtin_string_contains(LispObject *args, Environment *env);
static LispObject *builtin_string_index(LispObject *args, Environment *env);
static LispObject *builtin_string_match(LispObject *args, Environment *env);
static LispObject *builtin_substring(LispObject *args, Environment *env);
static LispObject *builtin_string_ref(LispObject *args, Environment *env);
static LispObject *builtin_string_prefix_question(LispObject *args, Environment *env);
static LispObject *builtin_string_replace(LispObject *args, Environment *env);
static LispObject *builtin_string_upcase(LispObject *args, Environment *env);
static LispObject *builtin_string_downcase(LispObject *args, Environment *env);
static LispObject *builtin_string_trim(LispObject *args, Environment *env);

/* Character operations */
static LispObject *builtin_char_question(LispObject *args, Environment *env);
static LispObject *builtin_char_code(LispObject *args, Environment *env);
static LispObject *builtin_code_char(LispObject *args, Environment *env);
static LispObject *builtin_char_to_string(LispObject *args, Environment *env);
static LispObject *builtin_string_to_char(LispObject *args, Environment *env);
static LispObject *builtin_char_eq(LispObject *args, Environment *env);
static LispObject *builtin_char_lt(LispObject *args, Environment *env);
static LispObject *builtin_char_gt(LispObject *args, Environment *env);
static LispObject *builtin_char_lte(LispObject *args, Environment *env);
static LispObject *builtin_char_gte(LispObject *args, Environment *env);
static LispObject *builtin_char_upcase(LispObject *args, Environment *env);
static LispObject *builtin_char_downcase(LispObject *args, Environment *env);
static LispObject *builtin_char_alphabetic(LispObject *args, Environment *env);
static LispObject *builtin_char_numeric(LispObject *args, Environment *env);
static LispObject *builtin_char_whitespace(LispObject *args, Environment *env);

/* Boolean operations */
static LispObject *builtin_not(LispObject *args, Environment *env);

/* List operations */
static LispObject *builtin_car(LispObject *args, Environment *env);
static LispObject *builtin_cdr(LispObject *args, Environment *env);
static LispObject *builtin_caar(LispObject *args, Environment *env);
static LispObject *builtin_cadr(LispObject *args, Environment *env);
static LispObject *builtin_cdar(LispObject *args, Environment *env);
static LispObject *builtin_cddr(LispObject *args, Environment *env);
static LispObject *builtin_caddr(LispObject *args, Environment *env);
static LispObject *builtin_cadddr(LispObject *args, Environment *env);
static LispObject *builtin_first(LispObject *args, Environment *env);
static LispObject *builtin_second(LispObject *args, Environment *env);
static LispObject *builtin_third(LispObject *args, Environment *env);
static LispObject *builtin_fourth(LispObject *args, Environment *env);
static LispObject *builtin_rest(LispObject *args, Environment *env);
static LispObject *builtin_set_car_bang(LispObject *args, Environment *env);
static LispObject *builtin_set_cdr_bang(LispObject *args, Environment *env);
static LispObject *builtin_cons(LispObject *args, Environment *env);
static LispObject *builtin_list(LispObject *args, Environment *env);
static LispObject *builtin_length(LispObject *args, Environment *env);
static LispObject *builtin_list_ref(LispObject *args, Environment *env);
static LispObject *builtin_reverse(LispObject *args, Environment *env);
static LispObject *builtin_append(LispObject *args, Environment *env);

/* Predicates */
static LispObject *builtin_null_question(LispObject *args, Environment *env);
static LispObject *builtin_atom_question(LispObject *args, Environment *env);
static LispObject *builtin_pair_question(LispObject *args, Environment *env);

/* Regex operations */
static LispObject *builtin_regex_match(LispObject *args, Environment *env);
static LispObject *builtin_regex_find(LispObject *args, Environment *env);
static LispObject *builtin_regex_find_all(LispObject *args, Environment *env);
static LispObject *builtin_regex_extract(LispObject *args, Environment *env);
static LispObject *builtin_regex_replace(LispObject *args, Environment *env);
static LispObject *builtin_regex_replace_all(LispObject *args, Environment *env);
static LispObject *builtin_regex_split(LispObject *args, Environment *env);
static LispObject *builtin_regex_escape(LispObject *args, Environment *env);
static LispObject *builtin_regex_valid(LispObject *args, Environment *env);

/* Integer operations */
static LispObject *builtin_quotient(LispObject *args, Environment *env);
static LispObject *builtin_remainder(LispObject *args, Environment *env);
static LispObject *builtin_even_question(LispObject *args, Environment *env);
static LispObject *builtin_odd_question(LispObject *args, Environment *env);

/* Hash table operations */
static LispObject *builtin_make_hash_table(LispObject *args, Environment *env);
static LispObject *builtin_hash_ref(LispObject *args, Environment *env);
static LispObject *builtin_hash_set_bang(LispObject *args, Environment *env);
static LispObject *builtin_hash_remove_bang(LispObject *args, Environment *env);
static LispObject *builtin_hash_clear_bang(LispObject *args, Environment *env);
static LispObject *builtin_hash_count(LispObject *args, Environment *env);
static LispObject *builtin_hash_keys(LispObject *args, Environment *env);
static LispObject *builtin_hash_values(LispObject *args, Environment *env);
static LispObject *builtin_hash_entries(LispObject *args, Environment *env);

/* File I/O operations */
static LispObject *builtin_open(LispObject *args, Environment *env);
static LispObject *builtin_close(LispObject *args, Environment *env);
static LispObject *builtin_read_line(LispObject *args, Environment *env);
static LispObject *builtin_write_line(LispObject *args, Environment *env);
static LispObject *builtin_read_sexp(LispObject *args, Environment *env);
static LispObject *builtin_read_json(LispObject *args, Environment *env);
static LispObject *builtin_delete_file(LispObject *args, Environment *env);
static LispObject *builtin_load(LispObject *args, Environment *env);
static LispObject *builtin_package_save(LispObject *args, Environment *env);
static LispObject *builtin_in_package(LispObject *args, Environment *env);
static LispObject *builtin_current_package(LispObject *args, Environment *env);
static LispObject *builtin_package_symbols(LispObject *args, Environment *env);
static LispObject *builtin_list_packages(LispObject *args, Environment *env);

/* String port operations */
static LispObject *builtin_open_input_string(LispObject *args, Environment *env);
static LispObject *builtin_port_peek_char(LispObject *args, Environment *env);
static LispObject *builtin_port_read_char(LispObject *args, Environment *env);
static LispObject *builtin_port_position(LispObject *args, Environment *env);
static LispObject *builtin_port_source(LispObject *args, Environment *env);
static LispObject *builtin_port_eof_question(LispObject *args, Environment *env);
static LispObject *builtin_string_port_question(LispObject *args, Environment *env);

/* Common Lisp printing operations */
static LispObject *builtin_princ(LispObject *args, Environment *env);
static LispObject *builtin_prin1(LispObject *args, Environment *env);
static LispObject *builtin_print_cl(LispObject *args, Environment *env);
static LispObject *builtin_format(LispObject *args, Environment *env);
static LispObject *builtin_terpri(LispObject *args, Environment *env);

/* Type predicates */
static LispObject *builtin_integer_question(LispObject *args, Environment *env);
static LispObject *builtin_boolean_question(LispObject *args, Environment *env);
static LispObject *builtin_number_question(LispObject *args, Environment *env);
static LispObject *builtin_vector_question(LispObject *args, Environment *env);
static LispObject *builtin_hash_table_question(LispObject *args, Environment *env);
static LispObject *builtin_string_question(LispObject *args, Environment *env);
static LispObject *builtin_symbol_question(LispObject *args, Environment *env);
static LispObject *builtin_keyword_question(LispObject *args, Environment *env);
static LispObject *builtin_list_question(LispObject *args, Environment *env);
static LispObject *builtin_function_question(LispObject *args, Environment *env);
static LispObject *builtin_macro_question(LispObject *args, Environment *env);
static LispObject *builtin_builtin_question(LispObject *args, Environment *env);
static LispObject *builtin_callable_question(LispObject *args, Environment *env);
static LispObject *builtin_function_params(LispObject *args, Environment *env);
static LispObject *builtin_function_body(LispObject *args, Environment *env);
static LispObject *builtin_function_name(LispObject *args, Environment *env);
/* Keyword operations */
static LispObject *builtin_keyword_name(LispObject *args, Environment *env);

/* Symbol operations */
static LispObject *builtin_symbol_to_string(LispObject *args, Environment *env);
static LispObject *builtin_string_to_symbol(LispObject *args, Environment *env);

/* Vector operations */
static LispObject *builtin_make_vector(LispObject *args, Environment *env);
static LispObject *builtin_vector_ref(LispObject *args, Environment *env);
static LispObject *builtin_vector_set_bang(LispObject *args, Environment *env);
static LispObject *builtin_vector_push_bang(LispObject *args, Environment *env);
static LispObject *builtin_vector_pop_bang(LispObject *args, Environment *env);

/* Alist and mapping operations */
static LispObject *builtin_assoc(LispObject *args, Environment *env);
static LispObject *builtin_assq(LispObject *args, Environment *env);
static LispObject *builtin_assv(LispObject *args, Environment *env);
static LispObject *builtin_alist_get(LispObject *args, Environment *env);

/* List membership */
static LispObject *builtin_member(LispObject *args, Environment *env);
static LispObject *builtin_memq(LispObject *args, Environment *env);
static LispObject *builtin_map(LispObject *args, Environment *env);
static LispObject *builtin_mapcar(LispObject *args, Environment *env);
static LispObject *builtin_filter(LispObject *args, Environment *env);
static LispObject *builtin_apply(LispObject *args, Environment *env);

/* Error introspection and handling */
static LispObject *builtin_error_question(LispObject *args, Environment *env);
static LispObject *builtin_error_type(LispObject *args, Environment *env);
static LispObject *builtin_error_message(LispObject *args, Environment *env);
static LispObject *builtin_error_stack(LispObject *args, Environment *env);
static LispObject *builtin_error_data(LispObject *args, Environment *env);
static LispObject *builtin_signal(LispObject *args, Environment *env);
static LispObject *builtin_error(LispObject *args, Environment *env);

/* Docstring introspection */
static LispObject *builtin_documentation(LispObject *args, Environment *env);
static LispObject *builtin_bound_p(LispObject *args, Environment *env);
static LispObject *builtin_set_documentation(LispObject *args, Environment *env);

/* Eval */
static LispObject *builtin_eval(LispObject *args, Environment *env);

/* Time */
static LispObject *builtin_current_time_ms(LispObject *args, Environment *env);

/* Profiling */
static LispObject *builtin_profile_start(LispObject *args, Environment *env);
static LispObject *builtin_profile_stop(LispObject *args, Environment *env);
static LispObject *builtin_profile_report(LispObject *args, Environment *env);
static LispObject *builtin_profile_reset(LispObject *args, Environment *env);

/* Equality predicates */
static LispObject *builtin_eq_predicate(LispObject *args, Environment *env);
static LispObject *builtin_equal_predicate(LispObject *args, Environment *env);
static LispObject *builtin_string_eq_predicate(LispObject *args, Environment *env);

/* Path expansion functions */
static LispObject *builtin_home_directory(LispObject *args, Environment *env);
static LispObject *builtin_expand_path(LispObject *args, Environment *env);

/* Environment and filesystem functions */
static LispObject *builtin_getenv(LispObject *args, Environment *env);
static LispObject *builtin_data_directory(LispObject *args, Environment *env);
static LispObject *builtin_file_exists_question(LispObject *args, Environment *env);
static LispObject *builtin_mkdir(LispObject *args, Environment *env);

/* Helper for wildcard matching */
static int match_char_class(const char **pattern, char c);
static int wildcard_match(const char *pattern, const char *str);

/* Create bloom-lisp-version alist with version information.
 * Called during initialization to define the bloom-lisp-version variable.
 */
static LispObject *create_bloom_lisp_version_alist(void)
{
    /* Build association list using tail-pointer pattern */
    LispObject *result = NIL;
    LispObject *tail = NULL;

    /* Helper macro to add key-value pairs */
#define ADD_VERSION_PAIR(key_name, value_expr)            \
    do {                                                  \
        LispObject *key = lisp_make_symbol(key_name);     \
        LispObject *value = (value_expr);                 \
        LispObject *pair = lisp_make_cons(key, value);    \
        LispObject *new_cons = lisp_make_cons(pair, NIL); \
        if (result == NIL) {                              \
            result = new_cons;                            \
            tail = new_cons;                              \
        } else {                                          \
            tail->value.cons.cdr = new_cons;              \
            tail = new_cons;                              \
        }                                                 \
    } while (0)

    /* Version information */
    ADD_VERSION_PAIR("version", lisp_make_string(BLOOM_LISP_VERSION));
    ADD_VERSION_PAIR("major", lisp_make_integer(BLOOM_LISP_VERSION_MAJOR));
    ADD_VERSION_PAIR("minor", lisp_make_integer(BLOOM_LISP_VERSION_MINOR));
    ADD_VERSION_PAIR("patch", lisp_make_integer(BLOOM_LISP_VERSION_PATCH));

#undef ADD_VERSION_PAIR

    return result;
}

/* Helper macro to register a builtin and set its symbol's docstring */
#define REGISTER(name, func)                                                         \
    do {                                                                             \
        LispObject *sym = lisp_intern(name);                                         \
        const char *_doc = lookup_builtin_doc(name);                                 \
        if (_doc != NULL)                                                            \
            sym->value.symbol->docstring = (char *)_doc;                             \
        env_define(env, sym->value.symbol, lisp_make_builtin(func, name), pkg_core); \
    } while (0)

void register_builtins(Environment *env)
{
    REGISTER("+", builtin_add);
    REGISTER("-", builtin_subtract);
    REGISTER("*", builtin_multiply);
    REGISTER("/", builtin_divide);

    REGISTER(">", builtin_gt);
    REGISTER("<", builtin_lt);
    REGISTER("=", builtin_eq);
    REGISTER(">=", builtin_gte);
    REGISTER("<=", builtin_lte);

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
    REGISTER("string-contains?", builtin_string_contains);
    REGISTER("string-index", builtin_string_index);
    REGISTER("string-match?", builtin_string_match);
    REGISTER("substring", builtin_substring);
    REGISTER("string-ref", builtin_string_ref);
    REGISTER("string-prefix?", builtin_string_prefix_question);
    REGISTER("string-replace", builtin_string_replace);
    REGISTER("string-upcase", builtin_string_upcase);
    REGISTER("string-downcase", builtin_string_downcase);
    REGISTER("string-trim", builtin_string_trim);

    /* Character operations */
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
    REGISTER("char-alphabetic?", builtin_char_alphabetic);
    REGISTER("char-numeric?", builtin_char_numeric);
    REGISTER("char-whitespace?", builtin_char_whitespace);

    REGISTER("not", builtin_not);

    REGISTER("car", builtin_car);
    REGISTER("cdr", builtin_cdr);
    REGISTER("caar", builtin_caar);
    REGISTER("cadr", builtin_cadr);
    REGISTER("cdar", builtin_cdar);
    REGISTER("cddr", builtin_cddr);
    REGISTER("caddr", builtin_caddr);
    REGISTER("cadddr", builtin_cadddr);
    REGISTER("first", builtin_first);
    REGISTER("second", builtin_second);
    REGISTER("third", builtin_third);
    REGISTER("fourth", builtin_fourth);
    REGISTER("rest", builtin_rest);
    REGISTER("set-car!", builtin_set_car_bang);
    REGISTER("set-cdr!", builtin_set_cdr_bang);
    REGISTER("cons", builtin_cons);
    REGISTER("list", builtin_list);
    REGISTER("length", builtin_length);
    REGISTER("list-ref", builtin_list_ref);
    REGISTER("reverse", builtin_reverse);
    REGISTER("append", builtin_append);

    /* Alist operations */
    REGISTER("assoc", builtin_assoc);
    REGISTER("assq", builtin_assq);
    REGISTER("assv", builtin_assv);
    REGISTER("alist-get", builtin_alist_get);

    /* List membership */
    REGISTER("member", builtin_member);
    REGISTER("memq", builtin_memq);

    /* Equality predicates */
    REGISTER("eq?", builtin_eq_predicate);
    REGISTER("equal?", builtin_equal_predicate);
    REGISTER("string=?", builtin_string_eq_predicate);

    /* Mapping operations */
    REGISTER("map", builtin_map);
    REGISTER("mapcar", builtin_mapcar);
    REGISTER("filter", builtin_filter);
    REGISTER("apply", builtin_apply);

    REGISTER("null?", builtin_null_question);
    REGISTER("atom?", builtin_atom_question);
    REGISTER("pair?", builtin_pair_question);

    REGISTER("regex-match?", builtin_regex_match);
    REGISTER("regex-find", builtin_regex_find);
    REGISTER("regex-find-all", builtin_regex_find_all);
    REGISTER("regex-extract", builtin_regex_extract);
    REGISTER("regex-replace", builtin_regex_replace);
    REGISTER("regex-replace-all", builtin_regex_replace_all);
    REGISTER("regex-split", builtin_regex_split);
    REGISTER("regex-escape", builtin_regex_escape);
    REGISTER("regex-valid?", builtin_regex_valid);

    /* File I/O functions */
    REGISTER("open", builtin_open);
    REGISTER("close", builtin_close);
    REGISTER("read-line", builtin_read_line);
    REGISTER("write-line", builtin_write_line);
    REGISTER("read-sexp", builtin_read_sexp);
    REGISTER("read-json", builtin_read_json);
    REGISTER("delete-file", builtin_delete_file);
    REGISTER("load", builtin_load);

    /* Package functions */
    REGISTER("in-package", builtin_in_package);
    REGISTER("current-package", builtin_current_package);
    REGISTER("package-symbols", builtin_package_symbols);
    REGISTER("list-packages", builtin_list_packages);
    REGISTER("package-save", builtin_package_save);

    /* String port functions for O(1) sequential character access */
    REGISTER("open-input-string", builtin_open_input_string);
    REGISTER("port-peek-char", builtin_port_peek_char);
    REGISTER("port-read-char", builtin_port_read_char);
    REGISTER("port-position", builtin_port_position);
    REGISTER("port-source", builtin_port_source);
    REGISTER("port-eof?", builtin_port_eof_question);
    REGISTER("string-port?", builtin_string_port_question);

    /* Path expansion functions */
    REGISTER("home-directory", builtin_home_directory);
    REGISTER("expand-path", builtin_expand_path);

    /* Environment and filesystem functions */
    REGISTER("getenv", builtin_getenv);
    REGISTER("data-directory", builtin_data_directory);
    REGISTER("file-exists?", builtin_file_exists_question);
    REGISTER("mkdir", builtin_mkdir);

    /* Common Lisp printing functions */
    REGISTER("princ", builtin_princ);
    REGISTER("prin1", builtin_prin1);
    REGISTER("print", builtin_print_cl);
    REGISTER("format", builtin_format);
    REGISTER("terpri", builtin_terpri);

    /* Type predicates */
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

    /* Function introspection */
    REGISTER("function-params", builtin_function_params);
    REGISTER("function-body", builtin_function_body);
    REGISTER("function-name", builtin_function_name);

    /* Keyword operations */
    REGISTER("keyword-name", builtin_keyword_name);

    /* Symbol operations */
    REGISTER("symbol->string", builtin_symbol_to_string);
    REGISTER("string->symbol", builtin_string_to_symbol);

    /* Vector operations */
    REGISTER("make-vector", builtin_make_vector);
    REGISTER("vector-ref", builtin_vector_ref);
    REGISTER("vector-set!", builtin_vector_set_bang);
    REGISTER("vector-push!", builtin_vector_push_bang);
    REGISTER("vector-pop!", builtin_vector_pop_bang);

    /* Integer operations */
    REGISTER("quotient", builtin_quotient);
    REGISTER("remainder", builtin_remainder);
    REGISTER("even?", builtin_even_question);
    REGISTER("odd?", builtin_odd_question);

    /* Hash table operations */
    REGISTER("make-hash-table", builtin_make_hash_table);
    REGISTER("hash-ref", builtin_hash_ref);
    REGISTER("hash-set!", builtin_hash_set_bang);
    REGISTER("hash-remove!", builtin_hash_remove_bang);
    REGISTER("hash-clear!", builtin_hash_clear_bang);
    REGISTER("hash-count", builtin_hash_count);
    REGISTER("hash-keys", builtin_hash_keys);
    REGISTER("hash-values", builtin_hash_values);
    REGISTER("hash-entries", builtin_hash_entries);

    /* Error introspection and handling */
    REGISTER("error?", builtin_error_question);
    REGISTER("error-type", builtin_error_type);
    REGISTER("error-message", builtin_error_message);
    REGISTER("error-stack", builtin_error_stack);
    REGISTER("error-data", builtin_error_data);
    REGISTER("signal", builtin_signal);
    REGISTER("error", builtin_error);

    /* Docstring introspection functions */
    REGISTER("documentation", builtin_documentation);
    REGISTER("bound?", builtin_bound_p);
    REGISTER("set-documentation!", builtin_set_documentation);

    /* Eval */
    REGISTER("eval", builtin_eval);

    /* Time */
    REGISTER("current-time-ms", builtin_current_time_ms);

    /* Profiling */
    REGISTER("profile-start", builtin_profile_start);
    REGISTER("profile-stop", builtin_profile_stop);
    REGISTER("profile-report", builtin_profile_report);
    REGISTER("profile-reset", builtin_profile_reset);

    /* Define version information variable */
    env_define(env, lisp_intern("bloom-lisp-version")->value.symbol, create_bloom_lisp_version_alist(), pkg_core);
}

#undef REGISTER

/* Helper function to get numeric value */
static double get_numeric_value(LispObject *obj, int *is_integer)
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

/* String operations */
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

static LispObject *builtin_string_contains(LispObject *args, Environment *env)
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

static LispObject *builtin_string_match(LispObject *args, Environment *env)
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

/* Character operations */
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

static LispObject *builtin_char_alphabetic(LispObject *args, Environment *env)
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

static LispObject *builtin_char_numeric(LispObject *args, Environment *env)
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

static LispObject *builtin_char_whitespace(LispObject *args, Environment *env)
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

/* Boolean operations */
static LispObject *builtin_not(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("not");

    LispObject *arg = lisp_car(args);
    return lisp_is_truthy(arg) ? NIL : LISP_TRUE;
}

/* List operations */
static LispObject *builtin_car(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("car");

    LispObject *arg = lisp_car(args);
    if (arg == NIL) {
        return NIL;
    }

    if (arg->type != LISP_CONS) {
        return lisp_make_error("car requires a list");
    }

    return arg->value.cons.car;
}

static LispObject *builtin_cdr(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("cdr");

    LispObject *arg = lisp_car(args);
    if (arg == NIL) {
        return NIL;
    }

    if (arg->type != LISP_CONS) {
        return lisp_make_error("cdr requires a list");
    }

    return arg->value.cons.cdr;
}

/* c*r combination builtins */
#define DEFINE_CXR(cname, opname, func)                          \
    static LispObject *cname(LispObject *args, Environment *env) \
    {                                                            \
        (void)env;                                               \
        CHECK_ARGS_1(opname);                                    \
        return func(lisp_car(args));                             \
    }
DEFINE_CXR(builtin_caar, "caar", lisp_caar)
DEFINE_CXR(builtin_cadr, "cadr", lisp_cadr)
DEFINE_CXR(builtin_cdar, "cdar", lisp_cdar)
DEFINE_CXR(builtin_cddr, "cddr", lisp_cddr)
DEFINE_CXR(builtin_caddr, "caddr", lisp_caddr)
DEFINE_CXR(builtin_cadddr, "cadddr", lisp_cadddr)
#undef DEFINE_CXR

/* Readable list accessors */
#define DEFINE_ALIAS(cname, target)                              \
    static LispObject *cname(LispObject *args, Environment *env) \
    {                                                            \
        return target(args, env);                                \
    }
DEFINE_ALIAS(builtin_first, builtin_car)
DEFINE_ALIAS(builtin_second, builtin_cadr)
DEFINE_ALIAS(builtin_third, builtin_caddr)
DEFINE_ALIAS(builtin_fourth, builtin_cadddr)
DEFINE_ALIAS(builtin_rest, builtin_cdr)
#undef DEFINE_ALIAS

static LispObject *builtin_set_car_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("set-car!");

    LispObject *pair = lisp_car(args);
    LispObject *value = lisp_car(lisp_cdr(args));

    if (pair == NIL || pair->type != LISP_CONS) {
        return lisp_make_error("set-car! requires a cons cell as first argument");
    }

    pair->value.cons.car = value;
    return value;
}

static LispObject *builtin_set_cdr_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("set-cdr!");

    LispObject *pair = lisp_car(args);
    LispObject *value = lisp_car(lisp_cdr(args));

    if (pair == NIL || pair->type != LISP_CONS) {
        return lisp_make_error("set-cdr! requires a cons cell as first argument");
    }

    pair->value.cons.cdr = value;
    return value;
}

static LispObject *builtin_cons(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("cons");

    LispObject *car = lisp_car(args);
    LispObject *cdr = lisp_car(lisp_cdr(args));

    return lisp_make_cons(car, cdr);
}

static LispObject *builtin_list(LispObject *args, Environment *env)
{
    (void)env;
    return args;
}

static LispObject *builtin_length(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("length");

    LispObject *obj = lisp_car(args);

    switch (obj->type) {
    case LISP_CONS:
    case LISP_NIL:
    {
        /* List length */
        long long count = 0;
        LispObject *lst = obj;
        while (lst != NIL && lst != NULL) {
            count++;
            lst = lst->value.cons.cdr;
        }
        return lisp_make_integer(count);
    }
    case LISP_STRING:
    {
        /* String length (UTF-8 aware) */
        size_t char_count = utf8_strlen(obj->value.string);
        return lisp_make_integer((long long)char_count);
    }
    case LISP_VECTOR:
    {
        /* Vector length */
        return lisp_make_number((double)obj->value.vector.size);
    }
    default:
        return lisp_make_error("length requires a list, string, or vector");
    }
}

static LispObject *builtin_list_ref(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("list-ref");

    LispObject *lst = lisp_car(args);
    LispObject *index_obj = lisp_car(lisp_cdr(args));

    int index_is_integer;
    double index_val = get_numeric_value(index_obj, &index_is_integer);

    if (index_obj->type != LISP_INTEGER && index_obj->type != LISP_NUMBER) {
        return lisp_make_error("list-ref index must be a number");
    }

    long long index = (long long)index_val;
    if (index < 0) {
        return lisp_make_error("list-ref index must be non-negative");
    }

    /* Traverse the list, checking type at each step */
    for (long long i = 0; i < index && lst != NIL && lst != NULL; i++) {
        /* Type check before accessing cons fields */
        if (lst->type != LISP_CONS) {
            return lisp_make_error("list-ref: not a proper list");
        }
        lst = lst->value.cons.cdr;
    }

    if (lst == NIL || lst == NULL) {
        return lisp_make_error("list-ref index out of bounds");
    }

    /* Final type check before accessing car */
    if (lst->type != LISP_CONS) {
        return lisp_make_error("list-ref: not a proper list");
    }

    return lst->value.cons.car;
}

static LispObject *builtin_reverse(LispObject *args, Environment *env)
{
    (void)env;

    if (args == NULL || args == NIL) {
        return lisp_make_error("reverse: expected 1 argument");
    }

    LispObject *lst = lisp_car(args);

    /* Handle empty list */
    if (lst == NIL || lst == NULL) {
        return NIL;
    }

    /* Validate it's a list */
    if (lst->type != LISP_CONS) {
        return lisp_make_error("reverse: argument must be a list");
    }

    /* Build reversed list iteratively */
    LispObject *result = NIL;
    while (lst != NIL && lst != NULL && lst->type == LISP_CONS) {
        result = lisp_make_cons(lisp_car(lst), result);
        lst = lisp_cdr(lst);
    }

    return result;
}

static LispObject *builtin_append(LispObject *args, Environment *env)
{
    (void)env;

    /* No arguments: return empty list */
    if (args == NIL) {
        return NIL;
    }

    /* Build result by concatenating all argument lists */
    LispObject *result = NIL;
    LispObject *result_tail = NIL;

    /* Iterate through each argument */
    LispObject *current_arg = args;
    while (current_arg != NIL) {
        LispObject *list = lisp_car(current_arg);

        /* Each argument must be a list (or NIL) */
        if (list != NIL && list->type != LISP_CONS) {
            return lisp_make_error("append requires list arguments");
        }

        /* Copy elements from this list */
        LispObject *elem = list;
        while (elem != NIL && elem->type == LISP_CONS) {
            LIST_APPEND(result, result_tail, lisp_car(elem));
            elem = lisp_cdr(elem);
        }

        current_arg = lisp_cdr(current_arg);
    }

    return result;
}

/* Predicates (using DEFINE_TYPE_PRED-style inline) */
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
    return (arg->type != LISP_CONS) ? LISP_TRUE : NIL;
}

/* Regex helper: extract pattern + string args, compile regex.
 * Returns NULL on success (sets *re_out and *string_out).
 * Returns error LispObject on failure. */
static LispObject *regex_compile_args(LispObject *args, const char *name,
                                      pcre2_code **re_out,
                                      LispObject **string_out)
{
    if (args == NIL || lisp_cdr(args) == NIL) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s requires 2 arguments", name);
        return lisp_make_error(buf);
    }
    LispObject *pattern_obj = lisp_car(args);
    *string_out = lisp_car(lisp_cdr(args));
    if (pattern_obj->type != LISP_STRING || (*string_out)->type != LISP_STRING) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s requires strings", name);
        return lisp_make_error(buf);
    }
    char *error_msg = NULL;
    *re_out = compile_regex_pattern(pattern_obj->value.string, &error_msg);
    if (*re_out == NULL) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s: %s", name, error_msg);
        return lisp_make_error(buf);
    }
    return NULL;
}

static LispObject *builtin_regex_match(LispObject *args, Environment *env)
{
    (void)env;
    pcre2_code *re;
    LispObject *string_obj;
    LispObject *err = regex_compile_args(args, "regex-match?", &re, &string_obj);
    if (err)
        return err;

    pcre2_match_data *match_data = execute_regex(re, string_obj->value.string);
    int result = (match_data != NULL);
    free_regex_resources(re, match_data);
    return result ? LISP_TRUE : NIL;
}

static LispObject *builtin_regex_find(LispObject *args, Environment *env)
{
    (void)env;
    pcre2_code *re;
    LispObject *string_obj;
    LispObject *err = regex_compile_args(args, "regex-find", &re, &string_obj);
    if (err)
        return err;

    pcre2_match_data *match_data = execute_regex(re, string_obj->value.string);
    if (match_data == NULL) {
        free_regex_resources(re, NULL);
        return NIL;
    }
    char *matched = extract_capture(match_data, string_obj->value.string, 0);
    LispObject *result = matched ? lisp_make_string(matched) : NIL;
    free_regex_resources(re, match_data);
    return result;
}

static LispObject *builtin_regex_find_all(LispObject *args, Environment *env)
{
    (void)env;
    pcre2_code *re;
    LispObject *string_obj;
    LispObject *err = regex_compile_args(args, "regex-find-all", &re, &string_obj);
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

    pcre2_code_free(re);

    return result;
}

static LispObject *builtin_regex_extract(LispObject *args, Environment *env)
{
    (void)env;
    pcre2_code *re;
    LispObject *string_obj;
    LispObject *err = regex_compile_args(args, "regex-extract", &re, &string_obj);
    if (err)
        return err;

    pcre2_match_data *match_data = execute_regex(re, string_obj->value.string);
    if (match_data == NULL) {
        free_regex_resources(re, NULL);
        return NIL;
    }

    int capture_count = get_capture_count(re);
    LispObject *result = NIL;
    LispObject *tail = NULL;

    for (int i = 1; i <= capture_count; i++) {
        char *captured = extract_capture(match_data, string_obj->value.string, i);
        if (captured)
            LIST_APPEND(result, tail, lisp_make_string(captured));
    }

    free_regex_resources(re, match_data);
    return result;
}

static LispObject *builtin_regex_replace(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_3("regex-replace");

    LispObject *pattern_obj = lisp_car(args);
    LispObject *string_obj = lisp_car(lisp_cdr(args));
    LispObject *replacement_obj = lisp_car(lisp_cdr(lisp_cdr(args)));

    if (pattern_obj->type != LISP_STRING || replacement_obj->type != LISP_STRING ||
        string_obj->type != LISP_STRING)
        return lisp_make_error("regex-replace requires strings");

    char *error_msg = NULL;
    pcre2_code *re = compile_regex_pattern(pattern_obj->value.string, &error_msg);
    if (re == NULL) {
        char error[512];
        snprintf(error, sizeof(error), "regex-replace: %s", error_msg);
        return lisp_make_error(error);
    }

    size_t output_buffer_size = strlen(string_obj->value.string) * 2 + 256;
    size_t output_len = output_buffer_size;
    PCRE2_UCHAR *output = GC_malloc(output_buffer_size + 1); // +1 for null terminator

    int rc = pcre2_substitute(re, (PCRE2_SPTR)string_obj->value.string, PCRE2_ZERO_TERMINATED, 0, /* start offset */
                              PCRE2_SUBSTITUTE_GLOBAL,                                            /* options - replace all */
                              NULL,                                                               /* match data */
                              NULL,                                                               /* match context */
                              (PCRE2_SPTR)replacement_obj->value.string, PCRE2_ZERO_TERMINATED, output, &output_len);

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
    pcre2_code *re;
    LispObject *string_obj;
    LispObject *err = regex_compile_args(args, "regex-split", &re, &string_obj);
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

static LispObject *builtin_regex_valid(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("regex-valid?");
    LispObject *pattern_obj = lisp_car(args);
    if (pattern_obj->type != LISP_STRING)
        return lisp_make_error("regex-valid? requires a string");

    char *error_msg = NULL;
    pcre2_code *re = compile_regex_pattern(pattern_obj->value.string, &error_msg);

    if (re == NULL) {
        return NIL;
    }

    pcre2_code_free(re);
    return LISP_TRUE;
}

/* File I/O operations */

/* Helper function to create a file stream object */
LispObject *lisp_make_file_stream(FILE *file)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_FILE_STREAM;
    obj->value.file = file;
    return obj;
}

static LispObject *builtin_open(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_error("open requires at least 1 argument");
    }

    LispObject *filename_obj = lisp_car(args);
    if (filename_obj->type != LISP_STRING) {
        return lisp_make_error("open requires a string filename");
    }

    /* Default mode is "r" (read) */
    const char *mode = "r";
    if (lisp_cdr(args) != NIL && lisp_cdr(args) != NULL) {
        LispObject *mode_obj = lisp_car(lisp_cdr(args));
        if (mode_obj->type != LISP_STRING) {
            return lisp_make_error("open mode must be a string");
        }
        mode = mode_obj->value.string;
    }

    FILE *file = file_open(filename_obj->value.string, mode);
    if (file == NULL) {
        char error[512];
        snprintf(error, sizeof(error), "open: cannot open file '%s'", filename_obj->value.string);
        return lisp_make_error(error);
    }

    return lisp_make_file_stream(file);
}

static LispObject *builtin_close(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("close");

    LispObject *stream_obj = lisp_car(args);
    if (stream_obj->type != LISP_FILE_STREAM) {
        return lisp_make_error("close requires a file stream");
    }

    if (stream_obj->value.file != NULL) {
        fclose(stream_obj->value.file);
        stream_obj->value.file = NULL;
    }

    return NIL;
}

static LispObject *builtin_read_line(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("read-line");

    LispObject *stream_obj = lisp_car(args);
    if (stream_obj->type != LISP_FILE_STREAM) {
        return lisp_make_error("read-line requires a file stream");
    }

    FILE *file = stream_obj->value.file;
    if (file == NULL) {
        return lisp_make_error("read-line: file is closed");
    }

    /* Read line with dynamic buffer */
    size_t size = 256;
    char *buffer = GC_malloc(size);
    size_t pos = 0;

    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            /* Unix line ending */
            break;
        } else if (c == '\r') {
            /* Check for Windows (\r\n) or old Mac (\r) line ending */
            int next_c = fgetc(file);
            if (next_c == '\n') {
                /* Windows line ending \r\n */
                break;
            } else if (next_c != EOF) {
                /* Old Mac line ending \r, put back the character */
                ungetc(next_c, file);
            }
            /* We got \r, now break */
            break;
        }

        if (pos >= size - 1) {
            size *= 2;
            char *new_buffer = GC_malloc(size);
            strncpy(new_buffer, buffer, pos);
            buffer = new_buffer;
        }

        buffer[pos++] = c;
    }

    /* Check for EOF without newline */
    if (pos == 0 && c == EOF) {
        return NIL; /* End of file */
    }

    buffer[pos] = '\0';
    return lisp_make_string(buffer);
}

static LispObject *builtin_write_line(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_error("write-line requires at least 1 argument");
    }

    LispObject *stream_obj = lisp_car(args);
    if (stream_obj->type != LISP_FILE_STREAM) {
        return lisp_make_error("write-line requires a file stream");
    }

    FILE *file = stream_obj->value.file;
    if (file == NULL) {
        return lisp_make_error("write-line: file is closed");
    }

    LispObject *rest = lisp_cdr(args);
    if (rest == NIL) {
        return lisp_make_error("write-line requires a string to write");
    }

    LispObject *text_obj = lisp_car(rest);
    if (text_obj->type != LISP_STRING) {
        return lisp_make_error("write-line requires a string");
    }

    fprintf(file, "%s\n", text_obj->value.string);
    fflush(file);

    return text_obj;
}

/* Read S-expressions from file */
static LispObject *builtin_read_sexp(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("read-sexp");

    LispObject *arg = lisp_car(args);
    FILE *file = NULL;
    int should_close = 0;

    /* Check if argument is file stream or filename */
    if (arg->type == LISP_FILE_STREAM) {
        file = arg->value.file;
        if (file == NULL) {
            return lisp_make_error("read-sexp: file is closed");
        }
    } else if (arg->type == LISP_STRING) {
        /* Open file */
        file = file_open(arg->value.string, "r");
        if (file == NULL) {
            char error[512];
            snprintf(error, sizeof(error), "read-sexp: cannot open file '%s'", arg->value.string);
            return lisp_make_error(error);
        }
        should_close = 1;
    } else {
        return lisp_make_error("read-sexp requires a filename (string) or file stream");
    }

    /* Read entire file */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        if (should_close) {
            fclose(file);
        }
        return lisp_make_error("read-sexp: cannot determine file size");
    }

    char *buffer = GC_malloc(size + 1);
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';

    if (should_close) {
        fclose(file);
    }

    /* Parse all expressions */
    const char *input = buffer;
    LispObject *result = NIL;
    LispObject *last = NIL;

    while (*input) {
        /* Skip whitespace and comments */
        while (*input == ' ' || *input == '\t' || *input == '\n' || *input == '\r' || *input == ';') {
            if (*input == ';') {
                while (*input && *input != '\n') {
                    input++;
                }
            } else {
                input++;
            }
        }

        if (*input == '\0') {
            break;
        }

        /* Parse expression */
        LispObject *expr = lisp_read(&input);
        if (expr == NULL) {
            break;
        }

        if (expr->type == LISP_ERROR) {
            return expr;
        }

        LIST_APPEND(result, last, expr);
    }

    /* Return single expression if only one, otherwise return list */
    if (result != NIL && lisp_cdr(result) == NIL) {
        return lisp_car(result);
    }

    return result;
}

/* Read JSON from file - basic JSON parser */
static LispObject *builtin_read_json(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("read-json");

    LispObject *arg = lisp_car(args);
    FILE *file = NULL;
    int should_close = 0;

    /* Check if argument is file stream or filename */
    if (arg->type == LISP_FILE_STREAM) {
        file = arg->value.file;
        if (file == NULL) {
            return lisp_make_error("read-json: file is closed");
        }
    } else if (arg->type == LISP_STRING) {
        /* Open file */
        file = file_open(arg->value.string, "r");
        if (file == NULL) {
            char error[512];
            snprintf(error, sizeof(error), "read-json: cannot open file '%s'", arg->value.string);
            return lisp_make_error(error);
        }
        should_close = 1;
    } else {
        return lisp_make_error("read-json requires a filename (string) or file stream");
    }

    /* Read entire file */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        if (should_close) {
            fclose(file);
        }
        return lisp_make_error("read-json: cannot determine file size");
    }

    char *buffer = GC_malloc(size + 1);
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';

    if (should_close) {
        fclose(file);
    }

    /* Simple JSON parser - this is a basic implementation */
    /* Skip whitespace */
    const char *p = buffer;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
        p++;
    }

    if (*p == '\0') {
        return NIL;
    }

    /* Parse JSON value */
    LispObject *result = NULL;
    if (*p == '{') {
        /* JSON object -> hash table */
        result = lisp_make_hash_table();
        p++; /* Skip '{' */
        while (*p && *p != '}') {
            /* Skip whitespace */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
                p++;
            }
            if (*p == '}') {
                break;
            }

            /* Parse key (must be string) */
            if (*p != '"') {
                return lisp_make_error("read-json: object key must be a string");
            }
            p++; /* Skip '"' */
            const char *key_start = p;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p + 1)) {
                    p += 2; /* Skip escape sequence */
                } else {
                    p++;
                }
            }
            if (*p != '"') {
                return lisp_make_error("read-json: unterminated string");
            }
            size_t key_len = p - key_start;
            char *key = GC_malloc(key_len + 1);
            memcpy(key, key_start, key_len);
            key[key_len] = '\0';
            p++; /* Skip '"' */

            /* Skip whitespace */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
                p++;
            }
            if (*p != ':') {
                return lisp_make_error("read-json: expected ':' after key");
            }
            p++; /* Skip ':' */

            /* Skip whitespace */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
                p++;
            }

            /* Parse value (recursive) */
            LispObject *json_value = NULL;
            if (*p == '"') {
                /* String */
                p++;
                const char *val_start = p;
                while (*p && *p != '"') {
                    if (*p == '\\' && *(p + 1)) {
                        p += 2;
                    } else {
                        p++;
                    }
                }
                if (*p != '"') {
                    return lisp_make_error("read-json: unterminated string");
                }
                size_t val_len = p - val_start;
                char *val_str = GC_malloc(val_len + 1);
                memcpy(val_str, val_start, val_len);
                val_str[val_len] = '\0';
                json_value = lisp_make_string(val_str);
                p++; /* Skip '"' */
            } else if (*p == '{') {
                /* Nested object - simplified: return error for now */
                return lisp_make_error("read-json: nested objects not yet supported");
            } else if (*p == '[') {
                /* Array - simplified: return error for now */
                return lisp_make_error("read-json: arrays not yet supported");
            } else if (strncmp(p, "true", 4) == 0) {
                json_value = lisp_make_boolean(1);
                p += 4;
            } else if (strncmp(p, "false", 5) == 0) {
                json_value = NIL;
                p += 5;
            } else if (strncmp(p, "null", 4) == 0) {
                json_value = NIL;
                p += 4;
            } else if (isdigit(*p) || *p == '-') {
                /* Number - simplified parsing */
                const char *num_start = p;
                while (*p && (isdigit(*p) || *p == '.' || *p == '-' || *p == '+' || *p == 'e' || *p == 'E')) {
                    p++;
                }
                size_t num_len = p - num_start;
                char *num_str = GC_malloc(num_len + 1);
                memcpy(num_str, num_start, num_len);
                num_str[num_len] = '\0';
                if (strchr(num_str, '.') != NULL) {
                    json_value = lisp_make_number(atof(num_str));
                } else {
                    json_value = lisp_make_integer(atoll(num_str));
                }
            } else {
                return lisp_make_error("read-json: unexpected character");
            }

            /* Store in hash table */
            hash_table_set_entry(result, lisp_make_string(key), json_value);

            /* Skip whitespace */
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
                p++;
            }
            if (*p == ',') {
                p++; /* Skip ',' */
            } else if (*p != '}') {
                return lisp_make_error("read-json: expected ',' or '}'");
            }
        }
        if (*p == '}') {
            p++;
        }
    } else if (*p == '"') {
        /* JSON string */
        p++;
        const char *str_start = p;
        while (*p && *p != '"') {
            if (*p == '\\' && *(p + 1)) {
                p += 2;
            } else {
                p++;
            }
        }
        if (*p != '"') {
            return lisp_make_error("read-json: unterminated string");
        }
        size_t str_len = p - str_start;
        char *str = GC_malloc(str_len + 1);
        memcpy(str, str_start, str_len);
        str[str_len] = '\0';
        result = lisp_make_string(str);
        p++;
    } else if (strncmp(p, "true", 4) == 0) {
        result = lisp_make_boolean(1);
    } else if (strncmp(p, "false", 5) == 0) {
        result = NIL;
    } else if (strncmp(p, "null", 4) == 0) {
        result = NIL;
    } else if (isdigit(*p) || *p == '-') {
        /* Number */
        const char *num_start = p;
        while (*p && (isdigit(*p) || *p == '.' || *p == '-' || *p == '+' || *p == 'e' || *p == 'E')) {
            p++;
        }
        size_t num_len = p - num_start;
        char *num_str = GC_malloc(num_len + 1);
        memcpy(num_str, num_start, num_len);
        num_str[num_len] = '\0';
        if (strchr(num_str, '.') != NULL) {
            result = lisp_make_number(atof(num_str));
        } else {
            result = lisp_make_integer(atoll(num_str));
        }
    } else {
        return lisp_make_error("read-json: unsupported JSON value type");
    }

    return result;
}

/* Load and evaluate a Lisp file */
static LispObject *builtin_load(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("load");

    LispObject *filename_obj = lisp_car(args);
    if (filename_obj->type != LISP_STRING) {
        return lisp_make_error("load requires a string filename");
    }

    /* Save current *package* and restore after load */
    LispObject *saved_pkg = env_lookup(env, sym_star_package_star->value.symbol);

    LispObject *result = lisp_load_file(filename_obj->value.string, env);

    /* Restore *package* */
    if (saved_pkg) {
        env_set(env, sym_star_package_star->value.symbol, saved_pkg);
    }

    /* Return the result of the last expression evaluated, or nil if error */
    if (result && result->type == LISP_ERROR) {
        return result;
    }

    /* Return the last evaluated expression, or nil if file was empty */
    return result ? result : NIL;
}

/* Check if a value needs constructor forms (not just lisp_print + quote) */
static int needs_constructor(LispObject *obj)
{
    if (obj == NULL || obj == NIL)
        return 0;
    switch (obj->type) {
    case LISP_LAMBDA:
    case LISP_MACRO:
    case LISP_BUILTIN:
    case LISP_HASH_TABLE:
        return 1;
    case LISP_CONS:
    {
        LispObject *cur = obj;
        while (cur != NIL && cur->type == LISP_CONS) {
            if (needs_constructor(lisp_car(cur)))
                return 1;
            cur = lisp_cdr(cur);
        }
        if (cur != NIL && needs_constructor(cur))
            return 1;
        return 0;
    }
    case LISP_VECTOR:
        for (size_t i = 0; i < obj->value.vector.size; i++) {
            if (needs_constructor(obj->value.vector.items[i]))
                return 1;
        }
        return 0;
    default:
        return 0;
    }
}

/* Write a value expression to the file, using constructor forms if needed */
/* Write a string to FILE with proper escaping for Lisp readability */
static void write_escaped_string(FILE *f, const char *str)
{
    fputc('"', f);
    for (const char *p = str; *p; p++) {
        switch (*p) {
        case '\\':
            fputs("\\\\", f);
            break;
        case '"':
            fputs("\\\"", f);
            break;
        case '\n':
            fputs("\\n", f);
            break;
        case '\t':
            fputs("\\t", f);
            break;
        case '\r':
            fputs("\\r", f);
            break;
        default:
            fputc(*p, f);
            break;
        }
    }
    fputc('"', f);
}

/* Extracted lambda tracking for :defun mode */
typedef struct ExtractedLambda
{
    LispObject *obj;
    char name[256];
    struct ExtractedLambda *next;
} ExtractedLambda;

static void collect_lambdas(LispObject *val, ExtractedLambda **list, int *counter,
                            const char *binding_name)
{
    if (val == NULL || val == NIL)
        return;
    switch (val->type) {
    case LISP_LAMBDA:
    {
        /* Check if already collected (pointer equality) */
        for (ExtractedLambda *e = *list; e != NULL; e = e->next) {
            if (e->obj == val)
                return;
        }
        ExtractedLambda *entry = GC_MALLOC(sizeof(ExtractedLambda));
        entry->obj = val;
        if (val->value.lambda.name && strncmp(val->value.lambda.name, "lambda", 6) != 0) {
            snprintf(entry->name, sizeof(entry->name), "%s", val->value.lambda.name);
        } else {
            snprintf(entry->name, sizeof(entry->name), "%s--fn-%d", binding_name, (*counter)++);
        }
        entry->next = NULL;
        /* Append to end of list to maintain discovery order */
        if (*list == NULL) {
            *list = entry;
        } else {
            ExtractedLambda *tail = *list;
            while (tail->next != NULL)
                tail = tail->next;
            tail->next = entry;
        }
        /* Recurse into lambda body to find nested lambdas */
        LispObject *body = val->value.lambda.body;
        while (body != NIL && body->type == LISP_CONS) {
            collect_lambdas(lisp_car(body), list, counter, binding_name);
            body = lisp_cdr(body);
        }
        break;
    }
    case LISP_CONS:
    {
        LispObject *cur = val;
        while (cur != NIL && cur->type == LISP_CONS) {
            collect_lambdas(lisp_car(cur), list, counter, binding_name);
            cur = lisp_cdr(cur);
        }
        if (cur != NIL)
            collect_lambdas(cur, list, counter, binding_name);
        break;
    }
    case LISP_VECTOR:
        for (size_t i = 0; i < val->value.vector.size; i++) {
            collect_lambdas(val->value.vector.items[i], list, counter, binding_name);
        }
        break;
    case LISP_HASH_TABLE:
    {
        struct HashEntry **buckets = (struct HashEntry **)val->value.hash_table.buckets;
        size_t bucket_count = val->value.hash_table.bucket_count;
        for (size_t i = 0; i < bucket_count; i++) {
            struct HashEntry *entry = buckets[i];
            while (entry != NULL) {
                collect_lambdas(entry->value, list, counter, binding_name);
                entry = entry->next;
            }
        }
        break;
    }
    default:
        break;
    }
}

static const char *find_lambda_name(ExtractedLambda *list, LispObject *obj)
{
    for (ExtractedLambda *e = list; e != NULL; e = e->next) {
        if (e->obj == obj)
            return e->name;
    }
    return NULL;
}

static void write_defun(FILE *f, const char *name, LispObject *lambda)
{
    fprintf(f, "(defun %s %s", name, lisp_print(lambda->value.lambda.params));
    if (lambda->value.lambda.docstring) {
        fprintf(f, "\n  ");
        write_escaped_string(f, lambda->value.lambda.docstring);
    }
    LispObject *body = lambda->value.lambda.body;
    while (body != NIL && body->type == LISP_CONS) {
        fprintf(f, "\n  %s", lisp_print(lisp_car(body)));
        body = lisp_cdr(body);
    }
    fprintf(f, ")\n\n");
}

static void write_value_expr(FILE *f, LispObject *val, ExtractedLambda *extracted)
{
    if (val == NULL || val == NIL) {
        fprintf(f, "nil");
        return;
    }
    switch (val->type) {
    case LISP_LAMBDA:
    {
        /* In defun mode, emit extracted name instead of inline lambda */
        const char *extracted_name = extracted ? find_lambda_name(extracted, val) : NULL;
        if (extracted_name) {
            fprintf(f, "%s", extracted_name);
        } else {
            fprintf(f, "(lambda %s", lisp_print(val->value.lambda.params));
            /* Emit docstring if present */
            if (val->value.lambda.docstring) {
                fprintf(f, " ");
                write_escaped_string(f, val->value.lambda.docstring);
            }
            LispObject *body = val->value.lambda.body;
            while (body != NIL && body->type == LISP_CONS) {
                fprintf(f, " %s", lisp_print(lisp_car(body)));
                body = lisp_cdr(body);
            }
            fprintf(f, ")");
        }
        break;
    }
    case LISP_MACRO:
    {
        /* Macros are handled at the define level, but if we get here
           (e.g. nested in a list), emit as a lambda-like form */
        fprintf(f, "(lambda %s", lisp_print(val->value.macro.params));
        if (val->value.macro.docstring) {
            fprintf(f, " ");
            write_escaped_string(f, val->value.macro.docstring);
        }
        LispObject *body = val->value.macro.body;
        while (body != NIL && body->type == LISP_CONS) {
            fprintf(f, " %s", lisp_print(lisp_car(body)));
            body = lisp_cdr(body);
        }
        fprintf(f, ")");
        break;
    }
    case LISP_BUILTIN:
        /* Emit the builtin's name as a symbol reference */
        fprintf(f, "%s", val->value.builtin.name);
        break;
    case LISP_HASH_TABLE:
    {
        fprintf(f, "(let ((ht (make-hash-table)))");
        struct HashEntry **buckets = (struct HashEntry **)val->value.hash_table.buckets;
        size_t bucket_count = val->value.hash_table.bucket_count;
        for (size_t i = 0; i < bucket_count; i++) {
            struct HashEntry *entry = buckets[i];
            while (entry != NULL) {
                fprintf(f, " (hash-set! ht ");
                write_value_expr(f, entry->key, extracted);
                fprintf(f, " ");
                write_value_expr(f, entry->value, extracted);
                fprintf(f, ")");
                entry = entry->next;
            }
        }
        fprintf(f, " ht)");
        break;
    }
    case LISP_CONS:
    {
        if (needs_constructor(val)) {
            /* Use (list ...) constructor form */
            fprintf(f, "(list");
            LispObject *cur = val;
            while (cur != NIL && cur->type == LISP_CONS) {
                fprintf(f, " ");
                write_value_expr(f, lisp_car(cur), extracted);
                cur = lisp_cdr(cur);
            }
            if (cur != NIL) {
                /* Dotted pair - can't use list, fall back to cons */
                /* This is a rare edge case */
                fprintf(f, " . ");
                write_value_expr(f, cur, extracted);
            }
            fprintf(f, ")");
        } else {
            fprintf(f, "'%s", lisp_print(val));
        }
        break;
    }
    case LISP_VECTOR:
    {
        if (needs_constructor(val)) {
            fprintf(f, "(vector");
            for (size_t i = 0; i < val->value.vector.size; i++) {
                fprintf(f, " ");
                write_value_expr(f, val->value.vector.items[i], extracted);
            }
            fprintf(f, ")");
        } else {
            fprintf(f, "%s", lisp_print(val));
        }
        break;
    }
    case LISP_BOOLEAN:
        fprintf(f, "%s", val->value.boolean ? "#t" : "#f");
        break;
    case LISP_SYMBOL:
        /* Quote the symbol to prevent evaluation */
        fprintf(f, "'%s", lisp_print(val));
        break;
    default:
        /* Numbers, integers, chars, strings, keywords — lisp_print roundtrips */
        fprintf(f, "%s", lisp_print(val));
        break;
    }
}

/* Save user session bindings to a file */
static LispObject *builtin_package_save(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_make_error("package-save requires 1-2 arguments");
    }

    LispObject *first_arg = lisp_car(args);
    LispObject *filename_obj = first_arg;
    Symbol *target_pkg = NULL;
    int defun_mode = 0;
    int format_mode = 0;

    /* Scan remaining args for package name, :defun, and :format keywords */
    LispObject *rest = lisp_cdr(args);
    while (rest != NIL && rest->type == LISP_CONS) {
        LispObject *arg = lisp_car(rest);
        if (arg->type == LISP_KEYWORD && strcmp(arg->value.symbol->name, ":defun") == 0) {
            defun_mode = 1;
        } else if (arg->type == LISP_KEYWORD && strcmp(arg->value.symbol->name, ":format") == 0) {
            format_mode = 1;
        } else if (arg->type == LISP_STRING) {
            target_pkg = lisp_intern(arg->value.string)->value.symbol;
        } else if (arg->type == LISP_SYMBOL) {
            target_pkg = arg->value.symbol;
        }
        rest = lisp_cdr(rest);
    }
    if (target_pkg == NULL) {
        target_pkg = env_current_package(env);
    }

    if (filename_obj->type != LISP_STRING) {
        return lisp_make_error("package-save requires a string filename");
    }

    FILE *f = file_open(filename_obj->value.string, "w");
    if (f == NULL) {
        char error[512];
        snprintf(error, sizeof(error), "package-save: cannot open '%s': %s", filename_obj->value.string,
                 strerror(errno));
        return lisp_make_error(error);
    }

    fprintf(f, ";; Bloom Lisp package saved by package-save\n");
    fprintf(f, ";; Load with: (load \"%s\")\n", filename_obj->value.string);
    fprintf(f, "(in-package \"%s\")\n\n", target_pkg->name);

    /* Collect bindings matching target package from all env frames */
    LispObject *bindings = NIL;
    Environment *e = env;
    while (e != NULL) {
        ENV_FOR_EACH_BINDING(e, binding)
        {
            if (binding->package == target_pkg) {
                /* Skip *package* itself */
                if (binding->symbol == sym_star_package_star->value.symbol)
                    continue;
                LispObject *pair = lisp_make_cons(lisp_make_string(binding->symbol->name), binding->value);
                bindings = lisp_make_cons(pair, bindings);
            }
        }
        e = e->parent;
    }

    /* Iterate in definition order */
    LispObject *cur = bindings;
    while (cur != NIL && cur->type == LISP_CONS) {
        LispObject *pair = lisp_car(cur);
        const char *name = lisp_car(pair)->value.string;
        LispObject *val = lisp_cdr(pair);

        /* Skip non-serializable types */
        if (val != NULL && (val->type == LISP_FILE_STREAM || val->type == LISP_STRING_PORT)) {
            fprintf(f, ";; Skipped %s (non-serializable %s)\n", name,
                    val->type == LISP_FILE_STREAM ? "file-stream" : "string-port");
            cur = lisp_cdr(cur);
            continue;
        }

        if (val != NULL && val->type == LISP_MACRO) {
            /* Emit defmacro form */
            fprintf(f, "(defmacro %s %s", name, lisp_print(val->value.macro.params));
            if (val->value.macro.docstring) {
                fprintf(f, "\n  ");
                write_escaped_string(f, val->value.macro.docstring);
            }
            LispObject *body = val->value.macro.body;
            while (body != NIL && body->type == LISP_CONS) {
                fprintf(f, "\n  %s", lisp_print(lisp_car(body)));
                body = lisp_cdr(body);
            }
            fprintf(f, ")\n\n");
        } else if (defun_mode && val != NULL && val->type == LISP_LAMBDA) {
            /* Top-level lambda: emit as defun directly */
            write_defun(f, name, val);
        } else if (defun_mode && val != NULL) {
            /* Extract nested lambdas, emit defuns, then define */
            ExtractedLambda *extracted = NULL;
            int counter = 0;
            collect_lambdas(val, &extracted, &counter, name);
            for (ExtractedLambda *e = extracted; e != NULL; e = e->next) {
                write_defun(f, e->name, e->obj);
            }
            fprintf(f, "(define %s ", name);
            write_value_expr(f, val, extracted);
            fprintf(f, ")\n\n");
        } else {
            fprintf(f, "(define %s ", name);
            write_value_expr(f, val, NULL);
            fprintf(f, ")\n\n");
        }

        cur = lisp_cdr(cur);
    }

    fclose(f);

    /* Optionally format the output using lisp-fmt's format-file */
    if (format_mode) {
        LispObject *fmt_sym = lisp_intern("format-file");
        LispObject *fmt_func = env_lookup(env, fmt_sym->value.symbol);
        if (fmt_func == NULL) {
            return lisp_make_error(
                "package-save: :format requires lisp-fmt.lisp to be loaded first");
        }
        LispObject *call = lisp_make_cons(fmt_sym, lisp_make_cons(filename_obj, NIL));
        LispObject *result = lisp_eval(call, env);
        if (result != NULL && result->type == LISP_ERROR) {
            return result;
        }
        if (result != NULL && result->type == LISP_STRING) {
            FILE *f2 = file_open(filename_obj->value.string, "w");
            if (f2 != NULL) {
                fputs(result->value.string, f2);
                fclose(f2);
            }
        }
    }

    return LISP_TRUE;
}

static LispObject *builtin_in_package(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("in-package");

    LispObject *arg = lisp_car(args);
    LispObject *pkg_sym_obj;

    if (arg->type == LISP_STRING) {
        pkg_sym_obj = lisp_intern(arg->value.string);
    } else if (arg->type == LISP_SYMBOL) {
        pkg_sym_obj = arg;
    } else {
        return lisp_make_error("in-package requires a string or symbol argument");
    }

    env_set(env, sym_star_package_star->value.symbol, pkg_sym_obj);
    return pkg_sym_obj;
}

static LispObject *builtin_current_package(LispObject *args, Environment *env)
{
    (void)args;
    Symbol *pkg = env_current_package(env);
    return lisp_intern(pkg->name);
}

static LispObject *builtin_package_symbols(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("package-symbols");

    LispObject *arg = lisp_car(args);
    Symbol *target_pkg;

    if (arg->type == LISP_STRING) {
        target_pkg = lisp_intern(arg->value.string)->value.symbol;
    } else if (arg->type == LISP_SYMBOL) {
        target_pkg = arg->value.symbol;
    } else {
        return lisp_make_error("package-symbols requires a string or symbol argument");
    }

    LispObject *result = NIL;
    Environment *e = env;
    while (e != NULL) {
        ENV_FOR_EACH_BINDING(e, binding)
        {
            if (binding->package == target_pkg) {
                LispObject *sym = lisp_intern(binding->symbol->name);
                LispObject *pair = lisp_make_cons(sym, binding->value);
                result = lisp_make_cons(pair, result);
            }
        }
        e = e->parent;
    }
    return result;
}

static LispObject *builtin_list_packages(LispObject *args, Environment *env)
{
    (void)args;

    /* Collect distinct package names from all bindings */
    LispObject *packages = NIL;
    Environment *e = env;
    while (e != NULL) {
        ENV_FOR_EACH_BINDING(e, binding)
        {
            if (binding->package != NULL) {
                /* Check if already in list (pointer comparison) */
                int found = 0;
                LispObject *cur = packages;
                while (cur != NIL && cur->type == LISP_CONS) {
                    if (lisp_car(cur)->type == LISP_SYMBOL &&
                        lisp_car(cur)->value.symbol == binding->package) {
                        found = 1;
                        break;
                    }
                    cur = lisp_cdr(cur);
                }
                if (!found) {
                    packages = lisp_make_cons(lisp_intern(binding->package->name), packages);
                }
            }
        }
        e = e->parent;
    }
    return packages;
}

/* Delete a file */
static LispObject *builtin_delete_file(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("delete-file");

    LispObject *filename_obj = lisp_car(args);
    if (filename_obj->type != LISP_STRING) {
        return lisp_make_error("delete-file requires a string filename");
    }

    /* Attempt to delete the file */
    if (file_remove(filename_obj->value.string) == 0) {
        /* Success */
        return NIL;
    } else {
        /* Failure - return error with errno message */
        char error[512];
        snprintf(error, sizeof(error), "delete-file: failed to delete '%s': %s", filename_obj->value.string,
                 strerror(errno));
        return lisp_make_error(error);
    }
}

/* String port operations for O(1) sequential character access */
static LispObject *builtin_open_input_string(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("open-input-string");

    LispObject *str = lisp_car(args);
    if (str->type != LISP_STRING) {
        return lisp_make_error("open-input-string requires a string");
    }

    return lisp_make_string_port(str->value.string);
}

static LispObject *builtin_port_peek_char(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-peek-char");

    LispObject *port = lisp_car(args);
    if (port->type != LISP_STRING_PORT) {
        return lisp_make_error("port-peek-char requires a string port");
    }

    if (port->value.string_port.byte_pos >= port->value.string_port.byte_len) {
        return NIL; /* EOF */
    }

    const char *ptr = port->value.string_port.buffer + port->value.string_port.byte_pos;
    return lisp_make_char(utf8_get_codepoint(ptr));
}

static LispObject *builtin_port_read_char(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-read-char");

    LispObject *port = lisp_car(args);
    if (port->type != LISP_STRING_PORT) {
        return lisp_make_error("port-read-char requires a string port");
    }

    if (port->value.string_port.byte_pos >= port->value.string_port.byte_len) {
        return NIL; /* EOF */
    }

    const char *ptr = port->value.string_port.buffer + port->value.string_port.byte_pos;
    unsigned int cp = utf8_get_codepoint(ptr);
    int bytes = utf8_char_bytes(ptr);
    port->value.string_port.byte_pos += bytes;
    port->value.string_port.char_pos++;
    return lisp_make_char(cp);
}

static LispObject *builtin_port_position(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-position");

    LispObject *port = lisp_car(args);
    if (port->type != LISP_STRING_PORT) {
        return lisp_make_error("port-position requires a string port");
    }

    return lisp_make_integer((long long)port->value.string_port.char_pos);
}

static LispObject *builtin_port_source(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-source");

    LispObject *port = lisp_car(args);
    if (port->type != LISP_STRING_PORT) {
        return lisp_make_error("port-source requires a string port");
    }

    return lisp_make_string(port->value.string_port.buffer);
}

static LispObject *builtin_port_eof_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-eof?");

    LispObject *port = lisp_car(args);
    if (port->type != LISP_STRING_PORT) {
        return lisp_make_error("port-eof? requires a string port");
    }

    if (port->value.string_port.byte_pos >= port->value.string_port.byte_len) {
        return LISP_TRUE;
    }
    return NIL;
}

static LispObject *builtin_string_port_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("string-port?");

    LispObject *obj = lisp_car(args);
    return (obj->type == LISP_STRING_PORT) ? LISP_TRUE : NIL;
}

static LispObject *builtin_princ(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("princ");

    LispObject *obj = lisp_car(args);
    lisp_princ(obj);

    /* Return the object (Common Lisp convention) */
    return obj;
}

static LispObject *builtin_prin1(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("prin1");

    LispObject *obj = lisp_car(args);
    lisp_prin1(obj);

    /* Return the object (Common Lisp convention) */
    return obj;
}

static LispObject *builtin_print_cl(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("print");

    LispObject *obj = lisp_car(args);
    lisp_print_cl(obj);

    /* Return the object (Common Lisp convention) */
    return obj;
}

static LispObject *builtin_format(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL || lisp_cdr(args) == NIL) {
        return lisp_make_error("format requires at least 2 arguments");
    }

    LispObject *dest = lisp_car(args);
    LispObject *format_str_obj = lisp_car(lisp_cdr(args));

    if (format_str_obj->type != LISP_STRING) {
        return lisp_make_error("format requires a string as format argument");
    }

    const char *format_str = format_str_obj->value.string;
    LispObject *format_args = lisp_cdr(lisp_cdr(args));

    /* Build output string */
    char *output = GC_malloc(4096);
    size_t output_len = 0;
    size_t output_capacity = 4096;

    LispObject *current_arg = format_args;

    for (const char *p = format_str; *p; p++) {
        if (*p == '~' && *(p + 1)) {
            p++;
            char directive = *p;

            if (directive == '%') {
                /* Newline */
                if (output_len + 1 >= output_capacity) {
                    output_capacity *= 2;
                    char *new_output = GC_malloc(output_capacity);
                    memcpy(new_output, output, output_len);
                    output = new_output;
                }
                output[output_len++] = '\n';
            } else if (directive == '~') {
                /* Literal tilde */
                if (output_len + 1 >= output_capacity) {
                    output_capacity *= 2;
                    char *new_output = GC_malloc(output_capacity);
                    memcpy(new_output, output, output_len);
                    output = new_output;
                }
                output[output_len++] = '~';
            } else if (directive == 'A' || directive == 'a') {
                /* Aesthetic - princ style (no quotes) */
                if (current_arg == NIL) {
                    return lisp_make_error("format: not enough arguments for format directives");
                }
                LispObject *arg = lisp_car(current_arg);
                char *arg_str = lisp_print(arg);
                /* Remove quotes from strings for aesthetic output */
                if (arg->type == LISP_STRING) {
                    arg_str = arg->value.string;
                }
                size_t arg_len = strlen(arg_str);
                if (output_len + arg_len >= output_capacity) {
                    while (output_len + arg_len >= output_capacity) {
                        output_capacity *= 2;
                    }
                    char *new_output = GC_malloc(output_capacity);
                    memcpy(new_output, output, output_len);
                    output = new_output;
                }
                memcpy(output + output_len, arg_str, arg_len);
                output_len += arg_len;
                current_arg = lisp_cdr(current_arg);
            } else if (directive == 'S' || directive == 's') {
                /* S-expression - prin1 style (with quotes) */
                if (current_arg == NIL) {
                    return lisp_make_error("format: not enough arguments for format directives");
                }
                LispObject *arg = lisp_car(current_arg);
                char *arg_str = lisp_print(arg);
                size_t arg_len = strlen(arg_str);
                if (output_len + arg_len >= output_capacity) {
                    while (output_len + arg_len >= output_capacity) {
                        output_capacity *= 2;
                    }
                    char *new_output = GC_malloc(output_capacity);
                    memcpy(new_output, output, output_len);
                    output = new_output;
                }
                memcpy(output + output_len, arg_str, arg_len);
                output_len += arg_len;
                current_arg = lisp_cdr(current_arg);
            } else {
                /* Unknown directive - just output it */
                if (output_len + 2 >= output_capacity) {
                    output_capacity *= 2;
                    char *new_output = GC_malloc(output_capacity);
                    memcpy(new_output, output, output_len);
                    output = new_output;
                }
                output[output_len++] = '~';
                output[output_len++] = directive;
            }
        } else {
            /* Regular character */
            if (output_len + 1 >= output_capacity) {
                output_capacity *= 2;
                char *new_output = GC_malloc(output_capacity);
                memcpy(new_output, output, output_len);
                output = new_output;
            }
            output[output_len++] = *p;
        }
    }

    output[output_len] = '\0';

    /* Handle destination */
    if (dest == NIL) {
        /* Return as string */
        return lisp_make_string(output);
    } else if (dest->type == LISP_BOOLEAN && dest->value.boolean) {
        /* Output to stdout */
        printf("%s", output);
        fflush(stdout);
        return NIL;
    } else {
        return lisp_make_error("format: invalid destination (use nil for string or #t for stdout)");
    }
}

static LispObject *builtin_terpri(LispObject *args, Environment *env)
{
    (void)env;
    (void)args;
    printf("\n");
    fflush(stdout);
    return NIL;
}

/* Path expansion functions */

/* Get user's home directory path (cross-platform)
 * Unix/Linux/macOS: $HOME
 * Windows: %USERPROFILE% or %HOMEDRIVE%%HOMEPATH%
 * Returns: String with home directory or NIL if not found
 */
static LispObject *builtin_home_directory(LispObject *args, Environment *env)
{
    (void)args; /* Takes no arguments */
    (void)env;

    const char *home = NULL;

#if defined(_WIN32) || defined(_WIN64)
    /* Windows: Try USERPROFILE first */
    home = getenv("USERPROFILE");

    /* Fallback: HOMEDRIVE + HOMEPATH */
    if (home == NULL) {
        const char *homedrive = getenv("HOMEDRIVE");
        const char *homepath = getenv("HOMEPATH");

        if (homedrive != NULL && homepath != NULL) {
            size_t len = strlen(homedrive) + strlen(homepath) + 1;
            char *combined = GC_malloc(len);
            snprintf(combined, len, "%s%s", homedrive, homepath);
            home = combined;
        }
    }
#else
    /* Unix/Linux/macOS: Use HOME */
    home = getenv("HOME");
#endif

    if (home == NULL) {
        return NIL; /* No home directory found */
    }

    return lisp_make_string(home);
}

/* Expand ~/ in file paths to home directory (cross-platform)
 * Takes: String (file path)
 * Returns: String (expanded path) or original if no ~/ prefix
 * Example: (expand-path "~/config.lisp") => "/home/user/config.lisp"
 */
static LispObject *builtin_expand_path(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("expand-path");

    LispObject *path_obj = lisp_car(args);
    if (path_obj->type != LISP_STRING) {
        return lisp_make_error("expand-path requires a string argument");
    }

    const char *path = path_obj->value.string;

    /* Check if path starts with ~/ */
    if (path[0] != '~' || (path[1] != '/' && path[1] != '\\' && path[1] != '\0')) {
        /* Not a ~/ path - return original */
        return path_obj;
    }

    /* Get home directory */
    LispObject *home_obj = builtin_home_directory(NIL, env);
    if (home_obj == NIL || home_obj->type != LISP_STRING) {
        /* No home directory - return error */
        return lisp_make_error("expand-path: cannot determine home directory");
    }

    const char *home = home_obj->value.string;

    /* Calculate expanded path length */
    /* If path is just "~", use home directory directly */
    if (path[1] == '\0') {
        return home_obj;
    }

    /* Skip ~/ or ~\ */
    const char *rest = path + 2;

    /* Build expanded path: home + / + rest */
    size_t home_len = strlen(home);
    size_t rest_len = strlen(rest);
    size_t total_len = home_len + 1 + rest_len + 1;

    char *expanded = GC_malloc(total_len);

#if defined(_WIN32) || defined(_WIN64)
    /* Windows: Use backslash separator */
    snprintf(expanded, total_len, "%s\\%s", home, rest);
#else
    /* Unix: Use forward slash separator */
    snprintf(expanded, total_len, "%s/%s", home, rest);
#endif

    return lisp_make_string(expanded);
}

/* Read an environment variable.
 * Takes: String (variable name)
 * Returns: String (value) or nil if not set
 */
static LispObject *builtin_getenv(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("getenv requires 1 argument");

    LispObject *name = lisp_car(args);
    if (name->type != LISP_STRING)
        return lisp_make_error("getenv requires a string argument");

    const char *value = getenv(name->value.string);
    if (!value)
        return NIL;

    return lisp_make_string(value);
}

/* Return platform-specific user data directory for an application.
 * Takes: String (app name)
 * Returns: String (path, not created) or error
 * Unix: $XDG_DATA_HOME/app or ~/.local/share/app
 * Windows: %LOCALAPPDATA%\app or %APPDATA%\app
 */
static LispObject *builtin_data_directory(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("data-directory requires 1 argument");

    LispObject *app = lisp_car(args);
    if (app->type != LISP_STRING)
        return lisp_make_error("data-directory requires a string argument");

    const char *app_name = app->value.string;
    char path[PATH_MAX];

#if defined(_WIN32) || defined(_WIN64)
    const char *dir = getenv("LOCALAPPDATA");
    if (!dir)
        dir = getenv("APPDATA");
    if (!dir)
        return lisp_make_error("data-directory: cannot determine data directory");
    snprintf(path, sizeof(path), "%s\\%s", dir, app_name);
#else
    const char *dir = getenv("XDG_DATA_HOME");
    if (dir && dir[0]) {
        snprintf(path, sizeof(path), "%s/%s", dir, app_name);
    } else {
        const char *home = getenv("HOME");
        if (!home)
            return lisp_make_error("data-directory: cannot determine home directory");
        snprintf(path, sizeof(path), "%s/.local/share/%s", home, app_name);
    }
#endif

    return lisp_make_string(path);
}

/* Check if a file or directory exists.
 * Takes: String (path)
 * Returns: #t or nil
 */
static LispObject *builtin_file_exists_question(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("file-exists? requires 1 argument");

    LispObject *path = lisp_car(args);
    if (path->type != LISP_STRING)
        return lisp_make_error("file-exists? requires a string argument");

    return file_exists(path->value.string) ? LISP_TRUE : NIL;
}

/* Create directory and all parents (mkdir -p).
 * Takes: String (path)
 * Returns: #t or error
 */
static LispObject *builtin_mkdir(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("mkdir requires 1 argument");

    LispObject *path = lisp_car(args);
    if (path->type != LISP_STRING)
        return lisp_make_error("mkdir requires a string argument");

    if (file_exists(path->value.string))
        return LISP_TRUE;

    if (file_mkdir_p(path->value.string) != 0) {
        char errbuf[256];
        snprintf(errbuf, sizeof(errbuf), "mkdir: %s: %s", path->value.string,
                 strerror(errno));
        return lisp_make_error(errbuf);
    }

    return LISP_TRUE;
}

/* Type predicates */
#define DEFINE_TYPE_PRED(cname, opname, check)                   \
    static LispObject *cname(LispObject *args, Environment *env) \
    {                                                            \
        (void)env;                                               \
        CHECK_ARGS_1(opname);                                    \
        LispObject *arg = lisp_car(args);                        \
        return (check) ? LISP_TRUE : NIL;                        \
    }
DEFINE_TYPE_PRED(builtin_integer_question, "integer?", arg->type == LISP_INTEGER)
DEFINE_TYPE_PRED(builtin_boolean_question, "boolean?", arg->type == LISP_BOOLEAN)
DEFINE_TYPE_PRED(builtin_number_question, "number?",
                 arg->type == LISP_NUMBER || arg->type == LISP_INTEGER)
DEFINE_TYPE_PRED(builtin_vector_question, "vector?", arg->type == LISP_VECTOR)
DEFINE_TYPE_PRED(builtin_hash_table_question, "hash-table?",
                 arg->type == LISP_HASH_TABLE)
DEFINE_TYPE_PRED(builtin_string_question, "string?", arg->type == LISP_STRING)
DEFINE_TYPE_PRED(builtin_symbol_question, "symbol?", arg->type == LISP_SYMBOL)
DEFINE_TYPE_PRED(builtin_list_question, "list?",
                 arg == NIL || arg->type == LISP_CONS)
DEFINE_TYPE_PRED(builtin_keyword_question, "keyword?",
                 arg->type == LISP_KEYWORD)
DEFINE_TYPE_PRED(builtin_function_question, "function?",
                 arg->type == LISP_LAMBDA)
DEFINE_TYPE_PRED(builtin_macro_question, "macro?", arg->type == LISP_MACRO)
DEFINE_TYPE_PRED(builtin_builtin_question, "builtin?",
                 arg->type == LISP_BUILTIN)
DEFINE_TYPE_PRED(builtin_callable_question, "callable?",
                 arg->type == LISP_LAMBDA || arg->type == LISP_MACRO ||
                     arg->type == LISP_BUILTIN)
#undef DEFINE_TYPE_PRED

static LispObject *builtin_function_params(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("function-params");
    LispObject *arg = lisp_car(args);
    if (arg->type == LISP_LAMBDA) {
        return arg->value.lambda.params ? arg->value.lambda.params : NIL;
    }
    if (arg->type == LISP_MACRO) {
        return arg->value.macro.params ? arg->value.macro.params : NIL;
    }
    return lisp_make_error("function-params requires a lambda or macro");
}

static LispObject *builtin_function_body(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("function-body");
    LispObject *arg = lisp_car(args);
    if (arg->type == LISP_LAMBDA) {
        return arg->value.lambda.body ? arg->value.lambda.body : NIL;
    }
    if (arg->type == LISP_MACRO) {
        return arg->value.macro.body ? arg->value.macro.body : NIL;
    }
    return lisp_make_error("function-body requires a lambda or macro");
}

static LispObject *builtin_function_name(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("function-name");
    LispObject *arg = lisp_car(args);
    if (arg->type == LISP_LAMBDA) {
        return arg->value.lambda.name ? lisp_make_string(arg->value.lambda.name) : NIL;
    }
    if (arg->type == LISP_MACRO) {
        return arg->value.macro.name ? lisp_make_string(arg->value.macro.name) : NIL;
    }
    if (arg->type == LISP_BUILTIN) {
        return arg->value.builtin.name ? lisp_make_string(arg->value.builtin.name) : NIL;
    }
    return lisp_make_error("function-name requires a lambda, macro, or builtin");
}

static LispObject *builtin_keyword_name(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("keyword-name");
    LispObject *arg = lisp_car(args);
    if (arg->type != LISP_KEYWORD) {
        return lisp_make_error("keyword-name requires a keyword argument");
    }
    /* Return name without the leading colon */
    const char *name = arg->value.symbol->name;
    if (name[0] == ':') {
        return lisp_make_string(name + 1);
    }
    return lisp_make_string(name);
}

static LispObject *builtin_pair_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("pair?");
    LispObject *arg = lisp_car(args);
    return (arg->type == LISP_CONS) ? LISP_TRUE : NIL;
}

/* Symbol operations */

static LispObject *builtin_symbol_to_string(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("symbol->string");
    LispObject *arg = lisp_car(args);
    if (arg->type != LISP_SYMBOL) {
        return lisp_make_error("symbol->string requires a symbol argument");
    }
    return lisp_make_string(arg->value.symbol->name);
}

static LispObject *builtin_string_to_symbol(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("string->symbol");
    LispObject *arg = lisp_car(args);
    if (arg->type != LISP_STRING) {
        return lisp_make_error("string->symbol requires a string argument");
    }
    return lisp_intern(arg->value.string);
}

/* Vector operations */
static LispObject *builtin_make_vector(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_error("make-vector requires at least 1 argument");
    }
    LispObject *size_obj = lisp_car(args);
    if (size_obj->type != LISP_NUMBER && size_obj->type != LISP_INTEGER) {
        return lisp_make_error("make-vector size must be a number");
    }
    size_t size = (size_t)(size_obj->type == LISP_INTEGER ? size_obj->value.integer : size_obj->value.number);

    LispObject *vec = lisp_make_vector(size);

    /* If initial value is provided, set all elements to it */
    if (lisp_cdr(args) != NIL) {
        LispObject *init_val = lisp_car(lisp_cdr(args));
        for (size_t i = 0; i < size; i++) {
            vec->value.vector.items[i] = init_val;
        }
        /* Set the size to match the capacity when initializing */
        vec->value.vector.size = size;
    }

    return vec;
}

static LispObject *builtin_vector_ref(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("vector-ref");
    LispObject *vec_obj = lisp_car(args);
    if (vec_obj->type != LISP_VECTOR) {
        return lisp_make_error("vector-ref requires a vector");
    }
    LispObject *idx_obj = lisp_car(lisp_cdr(args));
    if (idx_obj->type != LISP_NUMBER && idx_obj->type != LISP_INTEGER) {
        return lisp_make_error("vector-ref index must be a number");
    }
    size_t idx = (size_t)(idx_obj->type == LISP_INTEGER ? idx_obj->value.integer : idx_obj->value.number);
    if (idx >= vec_obj->value.vector.size) {
        return lisp_make_error("vector-ref: index out of bounds");
    }
    return vec_obj->value.vector.items[idx];
}

static LispObject *builtin_vector_set_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_3("vector-set!");
    LispObject *vec_obj = lisp_car(args);
    if (vec_obj->type != LISP_VECTOR) {
        return lisp_make_error("vector-set! requires a vector");
    }
    LispObject *idx_obj = lisp_car(lisp_cdr(args));
    if (idx_obj->type != LISP_NUMBER && idx_obj->type != LISP_INTEGER) {
        return lisp_make_error("vector-set! index must be a number");
    }
    size_t idx = (size_t)(idx_obj->type == LISP_INTEGER ? idx_obj->value.integer : idx_obj->value.number);
    if (idx >= vec_obj->value.vector.size) {
        vec_obj->value.vector.size = idx + 1;
        /* Expand capacity if needed */
        if (vec_obj->value.vector.size > vec_obj->value.vector.capacity) {
            size_t new_capacity = vec_obj->value.vector.capacity;
            while (new_capacity < vec_obj->value.vector.size) {
                new_capacity *= 2;
            }
            LispObject **new_items = GC_malloc(sizeof(LispObject *) * new_capacity);
            for (size_t i = 0; i < vec_obj->value.vector.size - 1; i++) {
                new_items[i] = vec_obj->value.vector.items[i];
            }
            for (size_t i = vec_obj->value.vector.size - 1; i < new_capacity; i++) {
                new_items[i] = NIL;
            }
            vec_obj->value.vector.items = new_items;
            vec_obj->value.vector.capacity = new_capacity;
        }
    }
    LispObject *value = lisp_car(lisp_cdr(lisp_cdr(args)));
    vec_obj->value.vector.items[idx] = value;
    return value;
}

static LispObject *builtin_vector_push_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("vector-push!");
    LispObject *vec_obj = lisp_car(args);
    if (vec_obj->type != LISP_VECTOR) {
        return lisp_make_error("vector-push! requires a vector");
    }
    LispObject *value = lisp_car(lisp_cdr(args));
    /* Check if we need to expand */
    if (vec_obj->value.vector.size >= vec_obj->value.vector.capacity) {
        size_t new_capacity = vec_obj->value.vector.capacity * 2;
        LispObject **new_items = GC_malloc(sizeof(LispObject *) * new_capacity);
        for (size_t i = 0; i < vec_obj->value.vector.size; i++) {
            new_items[i] = vec_obj->value.vector.items[i];
        }
        for (size_t i = vec_obj->value.vector.size; i < new_capacity; i++) {
            new_items[i] = NIL;
        }
        vec_obj->value.vector.items = new_items;
        vec_obj->value.vector.capacity = new_capacity;
    }
    vec_obj->value.vector.items[vec_obj->value.vector.size] = value;
    vec_obj->value.vector.size++;
    return value;
}

static LispObject *builtin_vector_pop_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("vector-pop!");
    LispObject *vec_obj = lisp_car(args);
    if (vec_obj->type != LISP_VECTOR) {
        return lisp_make_error("vector-pop! requires a vector");
    }
    if (vec_obj->value.vector.size == 0) {
        return lisp_make_error("vector-pop!: cannot pop from empty vector");
    }
    vec_obj->value.vector.size--;
    return vec_obj->value.vector.items[vec_obj->value.vector.size];
}

/* Hash table operations */
static LispObject *builtin_make_hash_table(LispObject *args, Environment *env)
{
    (void)env;
    (void)args;
    return lisp_make_hash_table();
}

static LispObject *builtin_hash_ref(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("hash-ref");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-ref requires a hash table");
    }

    LispObject *key_obj = lisp_car(lisp_cdr(args));
    struct HashEntry *entry = hash_table_get_entry(table, key_obj);

    if (entry) {
        return entry->value;
    }

    return NIL; /* Key not found */
}

static LispObject *builtin_hash_set_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_3("hash-set!");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-set! requires a hash table");
    }

    LispObject *key_obj = lisp_car(lisp_cdr(args));
    LispObject *value = lisp_car(lisp_cdr(lisp_cdr(args)));

    hash_table_set_entry(table, key_obj, value);
    return value;
}

static LispObject *builtin_hash_remove_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("hash-remove!");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-remove! requires a hash table");
    }

    LispObject *key_obj = lisp_car(lisp_cdr(args));
    int removed = hash_table_remove_entry(table, key_obj);

    return removed ? LISP_TRUE : NIL;
}

static LispObject *builtin_hash_clear_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-clear!");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-clear! requires a hash table");
    }

    hash_table_clear(table);
    return NIL;
}

static LispObject *builtin_hash_count(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-count");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-count requires a hash table");
    }

    return lisp_make_number((double)table->value.hash_table.entry_count);
}

enum hash_iter_mode
{
    HASH_KEYS,
    HASH_VALUES,
    HASH_ENTRIES
};

static LispObject *hash_table_collect(LispObject *table, enum hash_iter_mode mode)
{
    struct HashEntry **buckets = (struct HashEntry **)table->value.hash_table.buckets;
    size_t bucket_count = table->value.hash_table.bucket_count;
    LispObject *result = NIL;
    LispObject *tail = NULL;

    for (size_t i = 0; i < bucket_count; i++) {
        for (struct HashEntry *e = buckets[i]; e != NULL; e = e->next) {
            LispObject *item;
            switch (mode) {
            case HASH_KEYS:
                item = e->key;
                break;
            case HASH_VALUES:
                item = e->value;
                break;
            case HASH_ENTRIES:
                item = lisp_make_cons(e->key, e->value);
                break;
            }
            LIST_APPEND(result, tail, item);
        }
    }
    return result;
}

static LispObject *builtin_hash_keys(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-keys");
    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE)
        return lisp_make_error("hash-keys requires a hash table");
    return hash_table_collect(table, HASH_KEYS);
}

static LispObject *builtin_hash_values(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-values");
    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE)
        return lisp_make_error("hash-values requires a hash table");
    return hash_table_collect(table, HASH_VALUES);
}

static LispObject *builtin_hash_entries(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-entries");
    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE)
        return lisp_make_error("hash-entries requires a hash table");
    return hash_table_collect(table, HASH_ENTRIES);
}

/* Alist operations */

/* Helper function to check deep structural equality (used by assoc, equal?, etc.) */
static int objects_equal_recursive(LispObject *a, LispObject *b)
{
    /* Fast path: pointer equality */
    if (a == b)
        return 1;

    /* NIL checks */
    if (a == NIL || b == NIL)
        return 0;

    /* Type mismatch */
    if (a->type != b->type)
        return 0;

    switch (a->type) {
    case LISP_NUMBER:
        return a->value.number == b->value.number;

    case LISP_INTEGER:
        return a->value.integer == b->value.integer;

    case LISP_CHAR:
        return a->value.character == b->value.character;

    case LISP_STRING:
        return strcmp(a->value.string, b->value.string) == 0;

    case LISP_SYMBOL:
        /* Symbols are interned, so pointer equality should work, but compare names for safety */
        return strcmp(a->value.symbol->name, b->value.symbol->name) == 0;

    case LISP_BOOLEAN:
        return a->value.boolean == b->value.boolean;

    case LISP_CONS:
        /* Recursive list comparison */
        return objects_equal_recursive(a->value.cons.car, b->value.cons.car) &&
               objects_equal_recursive(a->value.cons.cdr, b->value.cons.cdr);

    case LISP_VECTOR:
        /* Compare vector lengths */
        if (a->value.vector.size != b->value.vector.size) {
            return 0;
        }
        /* Compare each element */
        for (size_t i = 0; i < a->value.vector.size; i++) {
            if (!objects_equal_recursive(a->value.vector.items[i], b->value.vector.items[i])) {
                return 0;
            }
        }
        return 1;

    case LISP_HASH_TABLE:
        /* Compare hash table sizes */
        if (a->value.hash_table.entry_count != b->value.hash_table.entry_count) {
            return 0;
        }
        /* Compare each key-value pair */
        /* Iterate through all buckets in hash table a */
        struct HashEntry **a_buckets = (struct HashEntry **)a->value.hash_table.buckets;
        for (size_t i = 0; i < a->value.hash_table.bucket_count; i++) {
            struct HashEntry *entry = a_buckets[i];
            while (entry != NULL) {
                /* Look up the key in hash table b */
                struct HashEntry *b_entry = hash_table_get_entry(b, entry->key);
                if (b_entry == NULL) {
                    return 0; /* Key doesn't exist in b */
                }
                /* Compare values */
                if (!objects_equal_recursive(entry->value, b_entry->value)) {
                    return 0;
                }
                entry = entry->next;
            }
        }
        return 1;

    default:
        /* For other types (lambdas, builtins, etc.), use pointer equality */
        return 0;
    }
}

static int eq_pointer(LispObject *a, LispObject *b) { return a == b; }

static LispObject *alist_find(LispObject *key, LispObject *alist,
                              int (*eq)(LispObject *, LispObject *),
                              const char *name)
{
    while (alist != NIL && alist != NULL) {
        if (alist->type != LISP_CONS) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s requires an association list", name);
            return lisp_make_error(buf);
        }
        LispObject *pair = lisp_car(alist);
        if (pair != NIL && pair->type == LISP_CONS) {
            if (eq(key, lisp_car(pair)))
                return pair;
        }
        alist = lisp_cdr(alist);
    }
    return NIL;
}

static LispObject *builtin_assoc(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("assoc");
    return alist_find(lisp_car(args), lisp_car(lisp_cdr(args)),
                      objects_equal_recursive, "assoc");
}

static LispObject *builtin_assq(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("assq");
    return alist_find(lisp_car(args), lisp_car(lisp_cdr(args)),
                      eq_pointer, "assq");
}

static LispObject *builtin_assv(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("assv");
    return alist_find(lisp_car(args), lisp_car(lisp_cdr(args)),
                      objects_equal_recursive, "assv");
}

static LispObject *builtin_alist_get(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL || lisp_cdr(args) == NIL) {
        return lisp_make_error("alist-get requires at least 2 arguments");
    }

    LispObject *key = lisp_car(args);
    LispObject *alist = lisp_car(lisp_cdr(args));
    LispObject *default_val = NIL;

    /* Optional third argument is default value */
    if (lisp_cdr(lisp_cdr(args)) != NIL) {
        default_val = lisp_car(lisp_cdr(lisp_cdr(args)));
    }

    /* Iterate through association list */
    while (alist != NIL && alist != NULL) {
        if (alist->type != LISP_CONS) {
            return lisp_make_error("alist-get requires an association list");
        }

        LispObject *pair = lisp_car(alist);
        if (pair != NIL && pair->type == LISP_CONS) {
            LispObject *pair_key = lisp_car(pair);
            if (objects_equal_recursive(key, pair_key)) {
                return lisp_cdr(pair);
            }
        }

        alist = lisp_cdr(alist);
    }

    return default_val;
}

/* List membership */

static LispObject *builtin_member(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("member");

    LispObject *item = lisp_car(args);
    LispObject *list = lisp_car(lisp_cdr(args));

    /* Iterate through list using structural equality */
    while (list != NIL && list != NULL) {
        if (list->type != LISP_CONS) {
            return lisp_make_error("member requires a proper list");
        }

        LispObject *elem = lisp_car(list);
        if (objects_equal_recursive(item, elem)) {
            return list;
        }

        list = lisp_cdr(list);
    }

    return NIL;
}

static LispObject *builtin_memq(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("memq");

    LispObject *item = lisp_car(args);
    LispObject *list = lisp_car(lisp_cdr(args));

    /* Iterate through list using pointer equality */
    while (list != NIL && list != NULL) {
        if (list->type != LISP_CONS) {
            return lisp_make_error("memq requires a proper list");
        }

        LispObject *elem = lisp_car(list);
        if (item == elem) {
            return list;
        }

        list = lisp_cdr(list);
    }

    return NIL;
}

/* Equality predicates */

static LispObject *builtin_eq_predicate(LispObject *args, Environment *env)
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

static LispObject *builtin_equal_predicate(LispObject *args, Environment *env)
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

static LispObject *builtin_string_eq_predicate(LispObject *args, Environment *env)
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

/* Mapping operations */

/* Apply a unary function (builtin or lambda) to a single item */
static LispObject *apply_unary(LispObject *func, LispObject *item,
                               Environment *env, const char *name)
{
    if (func->type == LISP_BUILTIN) {
        return func->value.builtin.func(lisp_make_cons(item, NIL), env);
    }

    /* Lambda */
    Environment *lambda_env = env_create(func->value.lambda.closure);
    LispObject *params = func->value.lambda.params;
    if (params == NIL || params->type != LISP_CONS) {
        env_free(lambda_env);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: lambda must have at least one parameter", name);
        return lisp_make_error(buf);
    }
    LispObject *param = lisp_car(params);
    if (param->type != LISP_SYMBOL) {
        env_free(lambda_env);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: lambda parameter must be a symbol", name);
        return lisp_make_error(buf);
    }
    env_define(lambda_env, param->value.symbol, item, NULL);

    LispObject *body = func->value.lambda.body;
    LispObject *result = NIL;
    while (body != NIL && body != NULL) {
        result = lisp_eval(lisp_car(body), lambda_env);
        if (result->type == LISP_ERROR) {
            env_free(lambda_env);
            return result;
        }
        body = lisp_cdr(body);
    }
    env_free(lambda_env);
    return result;
}

static LispObject *builtin_map(LispObject *args, Environment *env)
{
    CHECK_ARGS_2("map");
    LispObject *func = lisp_car(args);
    LispObject *list = lisp_car(lisp_cdr(args));

    if (func->type != LISP_BUILTIN && func->type != LISP_LAMBDA)
        return lisp_make_error("map requires a function as first argument");

    LispObject *result = NIL;
    LispObject *tail = NULL;

    while (list != NIL && list != NULL) {
        if (list->type != LISP_CONS)
            return lisp_make_error("map requires a list");

        LispObject *mapped = apply_unary(func, lisp_car(list), env, "map");
        if (mapped->type == LISP_ERROR)
            return mapped;
        LIST_APPEND(result, tail, mapped);
        list = lisp_cdr(list);
    }
    return result;
}

static LispObject *builtin_mapcar(LispObject *args, Environment *env)
{
    return builtin_map(args, env);
}

static LispObject *builtin_filter(LispObject *args, Environment *env)
{
    CHECK_ARGS_2("filter");
    LispObject *func = lisp_car(args);
    LispObject *list = lisp_car(lisp_cdr(args));

    if (func->type != LISP_BUILTIN && func->type != LISP_LAMBDA)
        return lisp_make_error("filter requires a function as first argument");

    LispObject *result = NIL;
    LispObject *tail = NULL;

    while (list != NIL && list != NULL) {
        if (list->type != LISP_CONS)
            return lisp_make_error("filter requires a list");

        LispObject *item = lisp_car(list);
        LispObject *pred = apply_unary(func, item, env, "filter");
        if (pred->type == LISP_ERROR)
            return pred;

        if (pred != NIL &&
            !(pred->type == LISP_BOOLEAN && pred->value.boolean == 0))
            LIST_APPEND(result, tail, item);

        list = lisp_cdr(list);
    }
    return result;
}

static LispObject *builtin_apply(LispObject *args, Environment *env)
{
    CHECK_ARGS_2("apply");

    LispObject *func = lisp_car(args);
    LispObject *func_args = lisp_car(lisp_cdr(args));

    if (func->type != LISP_BUILTIN && func->type != LISP_LAMBDA) {
        return lisp_make_error("apply requires a function as first argument");
    }

    if (func_args != NIL && func_args->type != LISP_CONS) {
        return lisp_make_error("apply requires a list as second argument");
    }

    /* Call function directly without re-evaluating arguments */
    if (func->type == LISP_BUILTIN) {
        return func->value.builtin.func(func_args, env);
    }

    /* Lambda: create new environment and bind parameters */
    Environment *lambda_env = env_create(func->value.lambda.closure);

    /* Bind parameters to arguments */
    LispObject *params = func->value.lambda.params;
    LispObject *arg_list = func_args;

    while (params != NIL && params != NULL && params->type == LISP_CONS) {
        LispObject *param = lisp_car(params);

        /* Check for &optional (use interned symbol for fast pointer comparison) */
        if (param == sym_optional) {
            params = lisp_cdr(params);
            /* Bind remaining optional parameters */
            while (params != NIL && params != NULL && params->type == LISP_CONS) {
                param = lisp_car(params);
                if (param == sym_rest) {
                    break;
                }
                if (param->type == LISP_SYMBOL) {
                    if (arg_list != NIL && arg_list != NULL && arg_list->type == LISP_CONS) {
                        env_define(lambda_env, param->value.symbol, lisp_car(arg_list), NULL);
                        arg_list = lisp_cdr(arg_list);
                    } else {
                        env_define(lambda_env, param->value.symbol, NIL, NULL);
                    }
                }
                params = lisp_cdr(params);
            }
            continue;
        }

        /* Check for &rest (use interned symbol for fast pointer comparison) */
        if (param == sym_rest) {
            params = lisp_cdr(params);
            if (params != NIL && params->type == LISP_CONS) {
                LispObject *rest_param = lisp_car(params);
                if (rest_param->type == LISP_SYMBOL) {
                    env_define(lambda_env, rest_param->value.symbol, arg_list, NULL);
                }
            }
            break;
        }

        /* Regular required parameter */
        if (param->type == LISP_SYMBOL) {
            if (arg_list == NIL || arg_list == NULL) {
                env_free(lambda_env);
                return lisp_make_error("apply: too few arguments");
            }
            env_define(lambda_env, param->value.symbol, lisp_car(arg_list), NULL);
            arg_list = lisp_cdr(arg_list);
        }

        params = lisp_cdr(params);
    }

    /* Evaluate lambda body */
    LispObject *body = func->value.lambda.body;
    LispObject *result = NIL;
    while (body != NIL && body != NULL) {
        result = lisp_eval(lisp_car(body), lambda_env);
        if (result->type == LISP_ERROR) {
            env_free(lambda_env);
            return result;
        }
        body = lisp_cdr(body);
    }

    env_free(lambda_env);
    return result;
}

/* Error introspection and handling functions */

/* error? - Test if object is an error */
static LispObject *builtin_error_question(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_boolean(0);
    }
    LispObject *obj = lisp_car(args);
    return lisp_make_boolean(obj->type == LISP_ERROR);
}

/* error-type - Get error type symbol */
static LispObject *builtin_error_type(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_make_typed_error_simple("wrong-number-of-arguments", "error-type requires 1 argument", env);
    }
    LispObject *obj = lisp_car(args);
    if (obj->type != LISP_ERROR) {
        return lisp_make_typed_error_simple("wrong-type-argument", "error-type: argument must be an error", env);
    }
    /* Return error type (guaranteed to be a symbol) */
    if (obj->value.error_with_stack.error_type != NULL) {
        return obj->value.error_with_stack.error_type;
    }
    /* Fallback to 'error symbol if somehow NULL */
    return sym_error;
}

/* error-message - Get error message string */
static LispObject *builtin_error_message(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_make_typed_error_simple("wrong-number-of-arguments", "error-message requires 1 argument", env);
    }
    LispObject *obj = lisp_car(args);
    if (obj->type != LISP_ERROR) {
        return lisp_make_typed_error_simple("wrong-type-argument", "error-message: argument must be an error", env);
    }
    return lisp_make_string(obj->value.error_with_stack.message);
}

/* error-stack - Get error stack trace */
static LispObject *builtin_error_stack(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_make_typed_error_simple("wrong-number-of-arguments", "error-stack requires 1 argument", env);
    }
    LispObject *obj = lisp_car(args);
    if (obj->type != LISP_ERROR) {
        return lisp_make_typed_error_simple("wrong-type-argument", "error-stack: argument must be an error", env);
    }
    LispObject *stack = obj->value.error_with_stack.stack_trace;
    return (stack != NULL) ? stack : NIL;
}

/* error-data - Get error data payload */
static LispObject *builtin_error_data(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_make_typed_error_simple("wrong-number-of-arguments", "error-data requires 1 argument", env);
    }
    LispObject *obj = lisp_car(args);
    if (obj->type != LISP_ERROR) {
        return lisp_make_typed_error_simple("wrong-type-argument", "error-data: argument must be an error", env);
    }
    LispObject *data = obj->value.error_with_stack.data;
    return (data != NULL) ? data : NIL;
}

/* signal - Raise a typed error
 * (signal ERROR-SYMBOL DATA)
 */
static LispObject *builtin_signal(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_make_typed_error_simple("wrong-number-of-arguments", "signal requires at least 1 argument", env);
    }

    LispObject *error_type = lisp_car(args);
    if (error_type->type != LISP_SYMBOL) {
        return lisp_make_typed_error_simple("wrong-type-argument", "signal: first argument must be a symbol", env);
    }

    /* Get optional data argument */
    LispObject *rest = lisp_cdr(args);
    LispObject *data = (rest != NIL && rest != NULL) ? lisp_car(rest) : NIL;

    /* Build error message from data */
    char message[512];
    if (data != NIL && data->type == LISP_STRING) {
        snprintf(message, sizeof(message), "%s", data->value.string);
    } else if (data != NIL) {
        char *data_str = lisp_print(data);
        snprintf(message, sizeof(message), "%s: %s", error_type->value.symbol->name, data_str);
    } else {
        snprintf(message, sizeof(message), "%s", error_type->value.symbol->name);
    }

    return lisp_make_typed_error(error_type, message, data, env);
}

/* error - Convenience function to signal generic error
 * (error MESSAGE)
 */
static LispObject *builtin_error(LispObject *args, Environment *env)
{
    if (args == NIL) {
        return lisp_make_typed_error_simple("wrong-number-of-arguments", "error requires 1 argument", env);
    }

    LispObject *message_obj = lisp_car(args);
    const char *message;

    if (message_obj->type == LISP_STRING) {
        message = message_obj->value.string;
    } else {
        message = lisp_print(message_obj);
    }

    return lisp_make_typed_error_simple("error", message, env);
}

/* documentation - Get documentation string for a symbol
 * (documentation SYMBOL)
 * Returns the documentation string of the function or macro bound to SYMBOL,
 * or nil if none exists or if the symbol is unbound.
 * Similar to Emacs Lisp's documentation function.
 */
static LispObject *builtin_documentation(LispObject *args, Environment *env)
{
    (void)env; /* Docstrings are on symbols, not looked up in env */

    if (args == NIL) {
        return lisp_make_error("documentation requires at least 1 argument");
    }

    LispObject *symbol = lisp_car(args);

    if (symbol->type != LISP_SYMBOL) {
        return lisp_make_error("documentation requires a symbol");
    }

    /* Docstrings are stored on the symbol (set by REGISTER, define, or set-documentation!) */
    if (symbol->value.symbol->docstring != NULL) {
        return lisp_make_string(symbol->value.symbol->docstring);
    }

    return NIL;
}

static LispObject *builtin_bound_p(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("bound?");

    LispObject *symbol = lisp_car(args);

    if (symbol->type != LISP_SYMBOL) {
        return lisp_make_error("bound? requires a symbol");
    }

    /* Check if the symbol is bound */
    LispObject *value = env_lookup(env, symbol->value.symbol);

    return value != NULL ? LISP_TRUE : NIL;
}

static LispObject *builtin_eval(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("eval");

    LispObject *expr = lisp_car(args);

    /* Evaluate the expression in the current environment */
    return lisp_eval(expr, env);
}

static LispObject *builtin_current_time_ms(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

#ifdef _WIN32
    /* Windows: use GetTickCount64 for millisecond precision */
    return lisp_make_integer((long long)GetTickCount64());
#else
    /* POSIX: use clock_gettime with CLOCK_MONOTONIC */
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        long long ms = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        return lisp_make_integer(ms);
    }
    /* Fallback to time() if clock_gettime fails (unlikely) */
    return lisp_make_integer((long long)time(NULL) * 1000);
#endif
}

/* Profiling builtins */
static LispObject *builtin_profile_start(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;
    profile_reset();
    g_profile_state.enabled = 1;
    g_profile_state.start_time_ns = profile_get_time_ns();
    return LISP_TRUE;
}

static LispObject *builtin_profile_stop(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;
    g_profile_state.enabled = 0;
    return LISP_TRUE;
}

static LispObject *builtin_profile_report(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    /* Build alist of (function-name call-count total-ms) sorted by total time desc */
    /* First, count entries and collect into array for sorting */
    int count = 0;
    for (ProfileEntry *e = g_profile_state.entries; e != NULL; e = e->next) {
        count++;
    }

    if (count == 0) {
        return NIL;
    }

    /* Create array of pointers for sorting */
    ProfileEntry **arr = GC_malloc(sizeof(ProfileEntry *) * count);
    int i = 0;
    for (ProfileEntry *e = g_profile_state.entries; e != NULL; e = e->next) {
        arr[i++] = e;
    }

    /* Sort by total_time_ns descending (simple insertion sort, good enough for profiles) */
    for (int j = 1; j < count; j++) {
        ProfileEntry *key = arr[j];
        int k = j - 1;
        while (k >= 0 && arr[k]->total_time_ns < key->total_time_ns) {
            arr[k + 1] = arr[k];
            k--;
        }
        arr[k + 1] = key;
    }

    /* Build result list (reversed order to end up sorted) */
    LispObject *result = NIL;
    for (int j = count - 1; j >= 0; j--) {
        ProfileEntry *e = arr[j];
        /* Convert ns to ms (double for precision) */
        double total_ms = (double)e->total_time_ns / 1000000.0;

        /* Build entry: (name call-count total-ms) */
        LispObject *entry = lisp_make_cons(lisp_make_string(e->function_name),
                                           lisp_make_cons(lisp_make_integer((long long)e->call_count),
                                                          lisp_make_cons(lisp_make_number(total_ms), NIL)));
        result = lisp_make_cons(entry, result);
    }

    return result;
}

static LispObject *builtin_profile_reset(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;
    profile_reset();
    return LISP_TRUE;
}

static LispObject *builtin_set_documentation(LispObject *args, Environment *env)
{
    (void)env; /* Unused - docstrings are stored on symbols, not bindings */

    CHECK_ARGS_2("set-documentation!");

    LispObject *symbol = lisp_car(args);
    LispObject *docstring = lisp_car(lisp_cdr(args));

    if (symbol->type != LISP_SYMBOL) {
        return lisp_make_error("set-documentation! first argument must be a symbol");
    }

    if (docstring->type != LISP_STRING) {
        return lisp_make_error("set-documentation! second argument must be a string");
    }

    /* Set the docstring directly on the interned symbol */
    symbol->value.symbol->docstring = GC_strdup(docstring->value.string);

    return LISP_TRUE;
}

/* ========================================================================== */
/* Completion API for REPL and eval mode                                       */
/* ========================================================================== */

/*
 * Check if a value is callable (function, macro, special form, builtin).
 */
static int is_callable(LispObject *value)
{
    if (!value)
        return 0;

    switch (value->type) {
    case LISP_BUILTIN:
    case LISP_LAMBDA:
    case LISP_MACRO:
        return 1;
    default:
        return 0;
    }
}

/*
 * Check if a symbol name is a special form.
 */
static int is_special_form(const char *name)
{
    static const char *special_forms[] = {
        "if", "define", "set!", "lambda", "quote", "quasiquote",
        "let", "let*", "progn", "begin", "cond", "case",
        "and", "or", "do", "defmacro", "defun", "defvar",
        "defconst", "when", "unless", "condition-case", "unwind-protect", NULL
    };

    for (int i = 0; special_forms[i]; i++) {
        if (strcmp(name, special_forms[i]) == 0)
            return 1;
    }
    return 0;
}

/*
 * Case-insensitive prefix match.
 */
static int prefix_match(const char *str, const char *prefix)
{
    while (*prefix) {
        if (tolower((unsigned char)*str) != tolower((unsigned char)*prefix))
            return 0;
        str++;
        prefix++;
    }
    return 1;
}

/*
 * Get completion candidates from environment.
 *
 * Traverses env and all parent scopes, collecting symbol names that
 * match the given prefix. When ctx is LISP_COMPLETE_CALLABLE, only
 * callable symbols are included.
 *
 * Returns NULL-terminated array of strings. Caller must free with
 * lisp_free_completions().
 */
char **lisp_get_completions(Environment *env, const char *prefix, LispCompleteContext ctx)
{
    if (!env || !prefix)
        return NULL;

    /* First pass: count matches */
    int count = 0;
    int capacity = 64;
    char **results = malloc(capacity * sizeof(char *));
    if (!results)
        return NULL;

    /* Track seen names to avoid duplicates (shadowed bindings) */
    char **seen = malloc(capacity * sizeof(char *));
    int seen_count = 0;
    if (!seen) {
        free(results);
        return NULL;
    }

    /* Traverse environment chain */
    for (Environment *e = env; e != NULL; e = e->parent) {
        ENV_FOR_EACH_BINDING(e, b)
        {
            const char *bname = b->symbol->name;

            /* Check prefix match */
            if (!prefix_match(bname, prefix))
                continue;

            /* Check if already seen (shadowed) */
            int already_seen = 0;
            for (int i = 0; i < seen_count; i++) {
                if (strcmp(seen[i], bname) == 0) {
                    already_seen = 1;
                    break;
                }
            }
            if (already_seen)
                continue;

            /* Check callable filter */
            if (ctx == LISP_COMPLETE_CALLABLE) {
                if (!is_callable(b->value) && !is_special_form(bname))
                    continue;
            }

            /* Check variable filter (exclude callables) */
            if (ctx == LISP_COMPLETE_VARIABLE) {
                if (is_callable(b->value))
                    continue;
            }

            /* Add to results */
            if (count >= capacity - 1) {
                int new_capacity = capacity * 2;
                char **new_results = realloc(results, new_capacity * sizeof(char *));
                if (!new_results) {
                    for (int i = 0; i < count; i++)
                        free(results[i]);
                    free(results);
                    free(seen);
                    return NULL;
                }
                results = new_results;

                char **new_seen = realloc(seen, new_capacity * sizeof(char *));
                if (!new_seen) {
                    for (int i = 0; i < count; i++)
                        free(results[i]);
                    free(results);
                    return NULL;
                }
                seen = new_seen;
                capacity = new_capacity;
            }

            results[count] = strdup(bname);
            seen[seen_count++] = results[count]; /* Point to same string */
            count++;
        }
    }

    /* Add special forms (they're not in the environment, handled by eval) */
    /* Skip for variable context - special forms are not variables */
    if (ctx != LISP_COMPLETE_VARIABLE) {
        static const char *special_forms[] = {
            "if", "define", "set!", "lambda", "quote", "quasiquote",
            "let", "let*", "progn", "begin", "cond", "case",
            "and", "or", "do", "defmacro", "defun", "defvar",
            "defconst", "when", "unless", "condition-case", "unwind-protect", NULL
        };

        for (int i = 0; special_forms[i]; i++) {
            /* Check prefix match */
            if (!prefix_match(special_forms[i], prefix))
                continue;

            /* Check if already seen (some may be bound as macros too) */
            int already_seen = 0;
            for (int j = 0; j < seen_count; j++) {
                if (strcmp(seen[j], special_forms[i]) == 0) {
                    already_seen = 1;
                    break;
                }
            }
            if (already_seen)
                continue;

            /* Add to results */
            if (count >= capacity - 1) {
                int new_capacity = capacity * 2;
                char **new_results = realloc(results, new_capacity * sizeof(char *));
                if (!new_results) {
                    for (int j = 0; j < count; j++)
                        free(results[j]);
                    free(results);
                    free(seen);
                    return NULL;
                }
                results = new_results;

                char **new_seen = realloc(seen, new_capacity * sizeof(char *));
                if (!new_seen) {
                    for (int j = 0; j < count; j++)
                        free(results[j]);
                    free(results);
                    return NULL;
                }
                seen = new_seen;
                capacity = new_capacity;
            }

            results[count] = strdup(special_forms[i]);
            seen[seen_count++] = results[count];
            count++;
        }
    }

    /* Null-terminate */
    results[count] = NULL;

    free(seen);

    /* Return NULL if no matches */
    if (count == 0) {
        free(results);
        return NULL;
    }

    return results;
}

/*
 * Free completion array.
 */
void lisp_free_completions(char **completions)
{
    if (!completions)
        return;

    for (int i = 0; completions[i]; i++) {
        free(completions[i]);
    }
    free(completions);
}

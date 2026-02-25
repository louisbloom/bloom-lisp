#include "builtins_internal.h"

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

void register_builtins(Environment *env)
{
    register_arithmetic_builtins(env);
    register_comparison_builtins(env);
    register_strings_builtins(env);
    register_characters_builtins(env);
    register_lists_builtins(env);
    register_vectors_builtins(env);
    register_hash_tables_builtins(env);
    register_regex_builtins(env);
    register_file_io_builtins(env);
    register_string_ports_builtins(env);
    register_type_predicates_builtins(env);
    register_symbols_builtins(env);
    register_functions_builtins(env);
    register_printing_builtins(env);
    register_errors_builtins(env);
    register_packages_builtins(env);
    register_filesystem_builtins(env);
    register_time_profiling_builtins(env);

    /* Define version information variable */
    env_define(env, lisp_intern("bloom-lisp-version")->value.symbol, create_bloom_lisp_version_alist(), pkg_core);
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
    for (int i = 0; lisp_special_forms[i]; i++) {
        if (strcmp(name, lisp_special_forms[i]) == 0)
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
        for (int i = 0; lisp_special_forms[i]; i++) {
            /* Check prefix match */
            if (!prefix_match(lisp_special_forms[i], prefix))
                continue;

            /* Check if already seen (some may be bound as macros too) */
            int already_seen = 0;
            for (int j = 0; j < seen_count; j++) {
                if (strcmp(seen[j], lisp_special_forms[i]) == 0) {
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

            results[count] = strdup(lisp_special_forms[i]);
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

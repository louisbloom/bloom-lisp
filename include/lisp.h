#ifndef LISP_H
#define LISP_H

#include <gc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include "utf8.h"
#include <pcre2.h>

/* Forward declarations */
typedef struct LispObject LispObject;
typedef struct Environment Environment;
typedef struct CallStackFrame CallStackFrame;
typedef struct HandlerContext HandlerContext;
typedef struct Symbol Symbol;

/* Symbol structure - interned with optional docstring */
struct Symbol
{
    char *name;
    char *docstring; /* Documentation string (CommonMark format) */
};

/* Call stack frame structure */
struct CallStackFrame
{
    char *function_name;
    CallStackFrame *parent;
    uint64_t entry_time_ns; /* Entry timestamp for profiling */
};

/* Profiling data structures */
typedef struct ProfileEntry
{
    char *function_name;
    uint64_t call_count;
    uint64_t total_time_ns; /* Inclusive time (includes children) */
    struct ProfileEntry *next;
} ProfileEntry;

typedef struct
{
    int enabled;
    ProfileEntry *entries;  /* Linked list of profile entries */
    uint64_t start_time_ns; /* When profiling started */
} ProfileState;

extern ProfileState g_profile_state;

/* Profiling functions */
uint64_t profile_get_time_ns(void);
void profile_record(const char *function_name, uint64_t elapsed_ns);
void profile_reset(void);

/* Handler context for condition-case */
struct HandlerContext
{
    LispObject *handlers;       /* Assoc list: ((ERROR-SYMBOL . HANDLER-BODY) ...) */
    LispObject *error_var_name; /* Symbol: variable to bind error (or NIL) */
    Environment *handler_env;   /* Environment for handler evaluation */
    HandlerContext *parent;     /* Previous handler context */
};

/* Object types */
typedef enum
{
    LISP_NIL,
    LISP_NUMBER,
    LISP_INTEGER,
    LISP_CHAR,
    LISP_STRING,
    LISP_SYMBOL,
    LISP_KEYWORD,
    LISP_BOOLEAN,
    LISP_CONS,
    LISP_BUILTIN,
    LISP_LAMBDA,
    LISP_MACRO,
    LISP_ERROR,
    LISP_FILE_STREAM,
    LISP_VECTOR,
    LISP_HASH_TABLE,
    LISP_TAIL_CALL,
    LISP_STRING_PORT,
    LISP_REGEX
} LispType;

/* Built-in function pointer type */
typedef LispObject *(*BuiltinFunc)(LispObject *args, Environment *env);

/* Boxed-out info structs — separately allocated to keep sizeof(LispObject)
 * small (~24 bytes instead of ~80). Heap-only; immediates never reach them. */
typedef struct LambdaInfo
{
    LispObject *params;          /* Full parameter list (for display) */
    LispObject *required_params; /* List of required parameter symbols */
    LispObject *optional_params; /* List of optional parameter symbols */
    LispObject *rest_param;      /* Rest parameter symbol (or NULL) */
    int required_count;          /* Number of required params */
    int optional_count;          /* Number of optional params */
    LispObject *body;            /* Body expressions */
    Environment *closure;        /* Lexical environment */
    char *name;                  /* Optional function name for debugging */
    char *docstring;             /* Documentation string (CommonMark format) */
} LambdaInfo;

typedef struct MacroInfo
{
    LispObject *params;
    LispObject *body;
    Environment *closure;
    char *name;      /* Optional macro name for debugging */
    char *docstring; /* Documentation string (CommonMark format) */
} MacroInfo;

typedef struct ErrorInfo
{
    LispObject *error_type;  /* Symbol: 'error, 'division-by-zero, etc. */
    char *message;           /* Human-readable error message */
    LispObject *data;        /* Optional arbitrary data (can be NIL) */
    LispObject *stack_trace; /* List of function names */
    int caught;              /* Flag: if true, error won't propagate (caught by condition-case) */
} ErrorInfo;

typedef struct StringPortInfo
{
    char *buffer;    /* The string data */
    size_t byte_len; /* Total byte length (cached) */
    size_t char_len; /* Total character count (cached) */
    size_t byte_pos; /* Current byte position */
    size_t char_pos; /* Current character position */
} StringPortInfo;

typedef struct VectorInfo
{
    LispObject **items;
    size_t size;
    size_t capacity;
} VectorInfo;

typedef struct HashTableInfo
{
    void *buckets; /* Array of hash entry lists */
    size_t bucket_count;
    size_t entry_count;
    size_t capacity;
} HashTableInfo;

/* Lisp object structure */
struct LispObject
{
    LispType type;
    union
    {
        double number;
        long long integer;
        unsigned int character; /* Unicode codepoint for LISP_CHAR */
        char *string;
        Symbol *symbol; /* Interned symbol with name and docstring */
        int boolean;
        struct
        {
            LispObject *car;
            LispObject *cdr;
        } cons;
        struct
        {
            BuiltinFunc func;
            const char *name;
        } builtin;
        LambdaInfo *lambda; /* Boxed out — stable pointer */
        MacroInfo *macro;   /* Boxed out — stable pointer */
        char *error;        /* Legacy; LISP_ERROR uses error_with_stack */
        struct
        {
            FILE *fp;
            /* End-of-line style: "\n" (LF) or "\r\n" (CRLF). For read
             * streams, auto-detected on open by peeking up to 4 KB.
             * For write streams, defaults to "\n" and may be set via
             * set-stream-eol! or the optional third arg to open.
             * write-line always appends this suffix; write-string
             * translates each \n in its input to this string (Emacs
             * Lisp-style transparent EOL handling). */
            char eol[4];
        } file;
        VectorInfo *vector;          /* Boxed out — stable pointer */
        HashTableInfo *hash_table;   /* Boxed out — stable pointer */
        ErrorInfo *error_with_stack; /* Boxed out — stable pointer */
        struct
        {
            LispObject *func; /* Function to call in tail position */
            LispObject *args; /* Already-evaluated arguments */
        } tail_call;
        StringPortInfo *string_port; /* Boxed out — stable pointer */
        struct
        {
            pcre2_code *code; /* Compiled PCRE2 pattern; freed by GC finalizer */
        } regex;
    } value;
};

/* Environment structure for variable bindings */
#define ENV_INLINE_BUCKETS 8

struct Binding
{
    Symbol *symbol; /* Interned symbol (pointer comparison for lookup) */
    LispObject *value;
    Symbol *package;      /* Owning package symbol (NULL for local scopes) */
    struct Binding *next; /* Next binding in same hash bucket */
};

struct Environment
{
    struct Binding *inline_buckets[ENV_INLINE_BUCKETS];
    struct Binding **buckets; /* Points to inline_buckets or heap-allocated */
    size_t bucket_count;
    size_t binding_count;
    Environment *parent;
    CallStackFrame *call_stack;    /* Current call stack */
    HandlerContext *handler_stack; /* Active condition-case handlers */
};

/* Iterate all bindings in an environment frame */
#define ENV_FOR_EACH_BINDING(env, binding)                            \
    for (size_t _efi = 0; _efi < (env)->bucket_count; _efi++)         \
        for (struct Binding *binding = (env)->buckets[_efi]; binding; \
             binding = binding->next)

/* Global NIL object */
extern LispObject *NIL;

/* Global boolean objects */
extern LispObject *LISP_TRUE; /* #t */

/* Simple API */
Environment *lisp_init(void);
LispObject *lisp_eval_string(const char *code, Environment *env);
void lisp_cleanup(void);

/* Advanced API */
LispObject *lisp_read(const char **input);
LispObject *lisp_eval(LispObject *expr, Environment *env);

/* Apply a function to already-evaluated arguments.
 * func must be LISP_BUILTIN or LISP_LAMBDA.
 * args is a Lisp list of evaluated argument values (or NIL for no args). */
LispObject *lisp_apply(LispObject *func, LispObject *args, Environment *env);

/* Convenience wrappers for common arities */
LispObject *lisp_call_0(LispObject *func, Environment *env);
LispObject *lisp_call_1(LispObject *func, LispObject *arg1, Environment *env);
LispObject *lisp_call_2(LispObject *func, LispObject *arg1, LispObject *arg2, Environment *env);
LispObject *lisp_call_3(LispObject *func, LispObject *arg1, LispObject *arg2, LispObject *arg3, Environment *env);
char *lisp_print(LispObject *obj);
void lisp_princ(LispObject *obj);
void lisp_prin1(LispObject *obj);
void lisp_print_cl(LispObject *obj);
LispObject *lisp_load_file(const char *filename, Environment *env);

/* Object creation */
LispObject *lisp_make_number(double value);
LispObject *lisp_make_integer(long long value);
LispObject *lisp_make_char(unsigned int codepoint);
LispObject *lisp_make_string(const char *value);
LispObject *lisp_make_symbol(const char *name);
LispObject *lisp_make_boolean(int value);
LispObject *lisp_make_cons(LispObject *car, LispObject *cdr);
LispObject *lisp_make_error(const char *message);
LispObject *lisp_make_builtin(BuiltinFunc func, const char *name);
LispObject *lisp_make_lambda(LispObject *params, LispObject *body, Environment *closure, const char *name);
LispObject *lisp_make_lambda_ext(LispObject *params, LispObject *required_params, LispObject *optional_params,
                                 LispObject *rest_param, int required_count, int optional_count,
                                 LispObject *body, Environment *closure, const char *name);
LispObject *lisp_make_macro(LispObject *params, LispObject *body, Environment *closure, const char *name);
LispObject *lisp_make_file_stream(FILE *file);
LispObject *lisp_make_vector(size_t capacity);
LispObject *lisp_make_hash_table(void);
LispObject *lisp_make_tail_call(LispObject *func, LispObject *args);
LispObject *lisp_make_string_port(const char *str);
LispObject *lisp_make_regex(pcre2_code *code);

/* Symbol interning */
LispObject *lisp_intern(const char *name);
void lisp_set_docstring(const char *name, const char *docstring);

/* Keyword interning */
LispObject *lisp_make_keyword(const char *name);

/* X(c_name, lisp_name) — single source of truth for special forms */
#define SPECIAL_FORMS(X)                    \
    X(sym_quote, "quote")                   \
    X(sym_quasiquote, "quasiquote")         \
    X(sym_if, "if")                         \
    X(sym_define, "define")                 \
    X(sym_set, "set!")                      \
    X(sym_lambda, "lambda")                 \
    X(sym_defmacro, "defmacro")             \
    X(sym_let, "let")                       \
    X(sym_let_star, "let*")                 \
    X(sym_progn, "progn")                   \
    X(sym_do, "do")                         \
    X(sym_cond, "cond")                     \
    X(sym_case, "case")                     \
    X(sym_and, "and")                       \
    X(sym_or, "or")                         \
    X(sym_condition_case, "condition-case") \
    X(sym_unwind_protect, "unwind-protect")

/* Declare special form symbol globals */
#define DECLARE_SYM(c_name, lisp_name) extern LispObject *c_name;
SPECIAL_FORMS(DECLARE_SYM)
#undef DECLARE_SYM

/* Non-special-form pre-interned symbols */
extern LispObject *sym_unquote;
extern LispObject *sym_unquote_splicing;
extern LispObject *sym_else;
extern LispObject *sym_optional;
extern LispObject *sym_rest;
extern LispObject *sym_error;
extern LispObject *sym_package_ref;
extern LispObject *sym_star_package_star;

/* Name array for completion API */
extern const char *lisp_special_forms[];
extern Symbol *pkg_core;
extern Symbol *pkg_user;

/* Hash table entry structure */
struct HashEntry
{
    LispObject *key;
    LispObject *value;
    struct HashEntry *next;
};

/* Hash table operations */
struct HashEntry *hash_table_get_entry(LispObject *table, LispObject *key);
struct HashEntry *hash_table_set_entry(LispObject *table, LispObject *key, LispObject *value);
int hash_table_remove_entry(LispObject *table, LispObject *key);
void hash_table_clear(LispObject *table);
int hash_keys_equal(LispObject *a, LispObject *b);

/* Object utilities */
int lisp_is_truthy(LispObject *obj);
int lisp_is_list(LispObject *obj);
int lisp_is_callable(LispObject *obj);
size_t lisp_list_length(LispObject *list);

/* Environment functions */
Environment *env_create(Environment *parent);
void env_define(Environment *env, Symbol *sym, LispObject *value, Symbol *package);
LispObject *env_lookup(Environment *env, Symbol *sym);
LispObject *env_lookup_in_package(Environment *env, Symbol *sym, Symbol *package);
int env_set(Environment *env, Symbol *sym, LispObject *value);
void env_free(Environment *env);
Symbol *env_current_package(Environment *env);
/* Call stack functions */
void push_call_frame(Environment *env, const char *function_name);
void pop_call_frame(Environment *env);
LispObject *capture_call_stack(Environment *env);
LispObject *lisp_make_error_with_stack(const char *message, Environment *env);
LispObject *lisp_attach_stack_trace(LispObject *error, Environment *env);
LispObject *lisp_make_typed_error(LispObject *error_type, const char *message, LispObject *data, Environment *env);
LispObject *lisp_make_typed_error_simple(const char *error_type_name, const char *message, Environment *env);

/* Completion context for position-aware completion */
typedef enum
{
    LISP_COMPLETE_ALL,      /* Any symbol */
    LISP_COMPLETE_CALLABLE, /* Functions, macros, special forms only */
    LISP_COMPLETE_VARIABLE  /* Variables only (not callable) */
} LispCompleteContext;

/* Completion functions */
char **lisp_get_completions(Environment *env, const char *prefix, LispCompleteContext ctx);
void lisp_free_completions(char **completions);

/* List utilities */
LispObject *lisp_car(LispObject *obj);
LispObject *lisp_cdr(LispObject *obj);

/* c*r combination helpers */
LispObject *lisp_caar(LispObject *obj);
LispObject *lisp_cadr(LispObject *obj);
LispObject *lisp_cdar(LispObject *obj);
LispObject *lisp_cddr(LispObject *obj);
LispObject *lisp_caddr(LispObject *obj);
LispObject *lisp_cadddr(LispObject *obj);

/* Regex helper functions */
pcre2_code *compile_regex_pattern(const char *pattern, char **error_msg);
pcre2_match_data *execute_regex(pcre2_code *re, const char *subject);
char *extract_capture(pcre2_match_data *match_data, const char *subject, int capture_num);
int get_capture_count(pcre2_code *re);
void free_regex_resources(pcre2_code *re, pcre2_match_data *match_data);

/* Accessor macros for the LispObject value (LISP_TYPE, LISP_INT_VAL, ...).
 * Pulled in last so it can reference everything declared above. */
#include "lisp_value.h"

#endif /* LISP_H */

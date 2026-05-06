#include "../include/lisp.h"
#include "../include/file_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* NIL and LISP_TRUE are tagged immediates (NIL = 0x3, LISP_TRUE = 0xB) —
 * see lisp_value.h. No heap objects behind them. */

/* Global symbol intern table */
LispObject *symbol_table = NULL;

/* Global keyword intern table */
static LispObject *keyword_table = NULL;

/* Define special form symbol globals */
#define DEFINE_SYM(c_name, lisp_name) LispObject *c_name = NULL;
SPECIAL_FORMS(DEFINE_SYM)
#undef DEFINE_SYM

/* Non-special-form pre-interned symbols */
LispObject *sym_unquote = NULL;
LispObject *sym_unquote_splicing = NULL;
LispObject *sym_else = NULL;
LispObject *sym_optional = NULL;
LispObject *sym_rest = NULL;
LispObject *sym_error = NULL;
LispObject *sym_package_ref = NULL;
LispObject *sym_star_package_star = NULL;

/* Name array for completion API */
const char *lisp_special_forms[] = {
#define SF_NAME(c_name, lisp_name) lisp_name,
    SPECIAL_FORMS(SF_NAME)
#undef SF_NAME
        NULL
};
Symbol *pkg_core = NULL;
Symbol *pkg_user = NULL;

/* Object creation functions */
LispObject *lisp_make_number(double value)
{
    LispObject *obj = GC_malloc_atomic(sizeof(LispObject));
    obj->type = LISP_NUMBER;
    obj->value.number = value;
    return obj;
}

/* Fixnum range: signed values that survive the (n << 3) shift without
 * losing the sign bit. */
#define FIXNUM_MIN ((long long)-(1LL << 60))
#define FIXNUM_MAX ((long long)((1LL << 60) - 1))

LispObject *lisp_make_integer(long long value)
{
    if (value >= FIXNUM_MIN && value <= FIXNUM_MAX) {
        /* Tagged fixnum — no allocation. */
        return (LispObject *)((uintptr_t)((value << 3) | LISP_TAG_FIXNUM));
    }
    /* Big int falls back to the heap. */
    LispObject *obj = GC_malloc_atomic(sizeof(LispObject));
    obj->type = LISP_INTEGER;
    obj->value.integer = value;
    return obj;
}

LispObject *lisp_make_char(unsigned int codepoint)
{
    /* Codepoint is 21 bits; fits comfortably with a 3-bit tag. */
    return (LispObject *)((uintptr_t)(((uintptr_t)codepoint << 3) | LISP_TAG_CHAR));
}

LispObject *lisp_make_boolean(int value)
{
    return value ? LISP_TRUE : NIL; /* #f is NIL */
}

LispObject *lisp_make_string(const char *value)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_STRING;
    obj->value.string = GC_strdup(value);
    return obj;
}

LispObject *lisp_intern(const char *name)
{
    /* Initialize symbol table on first use */
    if (symbol_table == NULL) {
        symbol_table = lisp_make_hash_table();
    }

    /* Look up symbol in intern table */
    LispObject *name_key = lisp_make_string(name);
    struct HashEntry *entry = hash_table_get_entry(symbol_table, name_key);
    if (entry != NULL) {
        return entry->value; /* Return existing symbol */
    }

    /* Create new symbol struct */
    Symbol *sym = GC_malloc(sizeof(Symbol));
    sym->name = GC_strdup(name);
    sym->docstring = NULL;

    /* Create LispObject wrapper */
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_SYMBOL;
    obj->value.symbol = sym;

    /* Add to intern table */
    hash_table_set_entry(symbol_table, name_key, obj);

    return obj;
}

void lisp_set_docstring(const char *name, const char *docstring)
{
    LispObject *sym = lisp_intern(name);
    LISP_SYM_VAL(sym)->docstring = GC_strdup(docstring);
}

/* Keep old name for backward compatibility, but redirect to intern */
LispObject *lisp_make_symbol(const char *name)
{
    return lisp_intern(name);
}

LispObject *lisp_make_keyword(const char *name)
{
    /* Initialize keyword table on first use */
    if (keyword_table == NULL) {
        keyword_table = lisp_make_hash_table();
    }

    /* Look up keyword in intern table */
    LispObject *name_key = lisp_make_string(name);
    struct HashEntry *entry = hash_table_get_entry(keyword_table, name_key);
    if (entry != NULL) {
        return entry->value; /* Return existing keyword */
    }

    /* Create new symbol struct for keyword (reuses Symbol struct) */
    Symbol *sym = GC_malloc(sizeof(Symbol));
    sym->name = GC_strdup(name);
    sym->docstring = NULL;

    /* Create LispObject wrapper with LISP_KEYWORD type */
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_KEYWORD;
    obj->value.symbol = sym;

    /* Add to intern table */
    hash_table_set_entry(keyword_table, name_key, obj);

    return obj;
}

LispObject *lisp_make_cons(LispObject *car, LispObject *cdr)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_CONS;
    obj->value.cons.car = car;
    obj->value.cons.cdr = cdr;
    return obj;
}

/* New typed error creation function */
LispObject *lisp_make_typed_error(LispObject *error_type, const char *message, LispObject *data, Environment *env)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_ERROR;
    obj->value.error_with_stack = GC_malloc(sizeof(ErrorInfo));
    LISP_ERROR_TYPE(obj) = error_type;
    LISP_ERROR_MESSAGE(obj) = GC_strdup(message);
    LISP_ERROR_DATA(obj) = data;
    LISP_ERROR_STACK_TRACE(obj) = (env != NULL) ? capture_call_stack(env) : NIL;
    LISP_ERROR_CAUGHT(obj) = 0; /* Not caught initially - will propagate */
    return obj;
}

/* Convenience function for creating typed errors from strings */
LispObject *lisp_make_typed_error_simple(const char *error_type_name, const char *message, Environment *env)
{
    return lisp_make_typed_error(lisp_intern(error_type_name), message, NIL, env);
}

/* Backward-compatible error creation (uses 'error type) */
LispObject *lisp_make_error(const char *message)
{
    return lisp_make_typed_error(sym_error, message, NIL, NULL);
}

/* Backward-compatible error creation with stack trace */
LispObject *lisp_make_error_with_stack(const char *message, Environment *env)
{
    return lisp_make_typed_error(sym_error, message, NIL, env);
}

LispObject *lisp_attach_stack_trace(LispObject *error, Environment *env)
{
    if (LISP_TYPE(error) == LISP_ERROR) {
        if (LISP_ERROR_STACK_TRACE(error) == NIL || LISP_ERROR_STACK_TRACE(error) == NULL) {
            LISP_ERROR_STACK_TRACE(error) = capture_call_stack(env);
        }
    }
    return error;
}

LispObject *lisp_make_builtin(BuiltinFunc func, const char *name)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_BUILTIN;
    obj->value.builtin.func = func;
    obj->value.builtin.name = name;
    return obj;
}

LispObject *lisp_make_lambda(LispObject *params, LispObject *body, Environment *closure, const char *name)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_LAMBDA;
    obj->value.lambda = GC_malloc(sizeof(LambdaInfo));
    LISP_LAMBDA_PARAMS(obj) = params;
    LISP_LAMBDA_REQUIRED_PARAMS(obj) = params; /* For backward compat: all params are required */
    LISP_LAMBDA_OPTIONAL_PARAMS(obj) = NIL;
    LISP_LAMBDA_REST_PARAM(obj) = NULL;
    LISP_LAMBDA_REQUIRED_COUNT(obj) = 0; /* Will be computed during apply */
    LISP_LAMBDA_OPTIONAL_COUNT(obj) = 0;
    LISP_LAMBDA_BODY(obj) = body;
    LISP_LAMBDA_CLOSURE(obj) = closure;
    LISP_LAMBDA_NAME(obj) = name ? GC_strdup(name) : NULL;
    LISP_LAMBDA_DOCSTRING(obj) = NULL;
    return obj;
}

LispObject *lisp_make_lambda_ext(LispObject *params, LispObject *required_params, LispObject *optional_params,
                                 LispObject *rest_param, int required_count, int optional_count, LispObject *body,
                                 Environment *closure, const char *name)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_LAMBDA;
    obj->value.lambda = GC_malloc(sizeof(LambdaInfo));
    LISP_LAMBDA_PARAMS(obj) = params;
    LISP_LAMBDA_REQUIRED_PARAMS(obj) = required_params;
    LISP_LAMBDA_OPTIONAL_PARAMS(obj) = optional_params;
    LISP_LAMBDA_REST_PARAM(obj) = rest_param;
    LISP_LAMBDA_REQUIRED_COUNT(obj) = required_count;
    LISP_LAMBDA_OPTIONAL_COUNT(obj) = optional_count;
    LISP_LAMBDA_BODY(obj) = body;
    LISP_LAMBDA_CLOSURE(obj) = closure;
    LISP_LAMBDA_NAME(obj) = name ? GC_strdup(name) : NULL;
    LISP_LAMBDA_DOCSTRING(obj) = NULL;
    return obj;
}

LispObject *lisp_make_macro(LispObject *params, LispObject *body, Environment *closure, const char *name)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_MACRO;
    obj->value.macro = GC_malloc(sizeof(MacroInfo));
    LISP_MACRO_PARAMS(obj) = params;
    LISP_MACRO_BODY(obj) = body;
    LISP_MACRO_CLOSURE(obj) = closure;
    LISP_MACRO_NAME(obj) = name ? GC_strdup(name) : NULL;
    LISP_MACRO_DOCSTRING(obj) = NULL;
    return obj;
}

LispObject *lisp_make_tail_call(LispObject *func, LispObject *args)
{
    static LispObject tail_call_obj;
    tail_call_obj.type = LISP_TAIL_CALL;
    tail_call_obj.value.tail_call.func = func;
    tail_call_obj.value.tail_call.args = args;
    return &tail_call_obj;
}

LispObject *lisp_make_string_port(const char *str)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_STRING_PORT;
    obj->value.string_port = GC_malloc(sizeof(StringPortInfo));
    obj->value.string_port->buffer = GC_strdup(str);
    obj->value.string_port->byte_len = strlen(str);
    obj->value.string_port->char_len = utf8_strlen(str);
    obj->value.string_port->byte_pos = 0;
    obj->value.string_port->char_pos = 0;
    return obj;
}

/* PCRE2's compiled pattern is allocated outside Boehm GC, so the wrapping
 * LispObject installs a finalizer to release it when the wrapper becomes
 * unreachable. */
static void regex_finalizer(void *obj, void *cd)
{
    (void)cd;
    LispObject *r = obj;
    if (LISP_REGEX_CODE(r) != NULL) {
        pcre2_code_free(LISP_REGEX_CODE(r));
        r->value.regex.code = NULL;
    }
}

LispObject *lisp_make_regex(pcre2_code *code)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_REGEX;
    obj->value.regex.code = code;
    GC_register_finalizer(obj, regex_finalizer, NULL, NULL, NULL);
    return obj;
}

/* Object utilities */
int lisp_is_truthy(LispObject *obj)
{
    /* Traditional Lisp semantics: only nil is false, everything else is true */
    if (obj == NULL || obj == NIL) {
        return 0;
    }

    if (LISP_TYPE(obj) == LISP_NIL) {
        return 0;
    }

    /* Boolean false (#f) is falsy (since #f == nil) */
    if (LISP_TYPE(obj) == LISP_BOOLEAN && LISP_BOOL_VAL(obj) == 0) {
        return 0;
    }

    /* Everything else is truthy, including:
     * - Integer 0
     * - Float 0.0
     * - Empty strings ""
     * - Empty vectors
     * - Empty hash tables
     */
    return 1;
}

int lisp_is_list(LispObject *obj)
{
    if (obj == NIL)
        return 1;
    if (LISP_TYPE(obj) != LISP_CONS)
        return 0;

    while (obj != NIL && LISP_TYPE(obj) == LISP_CONS) {
        obj = LISP_CDR(obj);
    }

    return (obj == NIL);
}

int lisp_is_callable(LispObject *obj)
{
    return obj != NULL && obj != NIL &&
           (LISP_TYPE(obj) == LISP_BUILTIN || LISP_TYPE(obj) == LISP_LAMBDA);
}

size_t lisp_list_length(LispObject *list)
{
    size_t len = 0;
    while (list != NIL && LISP_TYPE(list) == LISP_CONS) {
        len++;
        list = LISP_CDR(list);
    }
    return len;
}

LispObject *lisp_make_vector(size_t capacity)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_VECTOR;
    obj->value.vector = GC_malloc(sizeof(VectorInfo));
    LISP_VECTOR_CAPACITY(obj) = capacity > 0 ? capacity : 8;
    LISP_VECTOR_SIZE(obj) = 0;
    LISP_VECTOR_ITEMS(obj) = GC_malloc(sizeof(LispObject *) * LISP_VECTOR_CAPACITY(obj));
    /* Initialize to NULL */
    for (size_t i = 0; i < LISP_VECTOR_CAPACITY(obj); i++) {
        LISP_VECTOR_ITEMS(obj)
        [i] = NIL;
    }
    return obj;
}

LispObject *lisp_make_hash_table(void)
{
    LispObject *obj = GC_malloc(sizeof(LispObject));
    obj->type = LISP_HASH_TABLE;
    obj->value.hash_table = GC_malloc(sizeof(HashTableInfo));
    LISP_HT_CAPACITY(obj) = 16;
    LISP_HT_BUCKET_COUNT(obj) = 16;
    LISP_HT_ENTRY_COUNT(obj) = 0;
    LISP_HT_BUCKETS(obj) = GC_malloc(sizeof(void *) * 16);
    /* Initialize buckets to NULL */
    for (size_t i = 0; i < 16; i++) {
        ((void **)LISP_HT_BUCKETS(obj))[i] = NULL;
    }
    return obj;
}

/* List utilities */
LispObject *lisp_car(LispObject *obj)
{
    if (obj == NIL || obj == NULL)
        return NIL;
    if (LISP_TYPE(obj) != LISP_CONS)
        return NIL;
    return LISP_CAR(obj);
}

LispObject *lisp_cdr(LispObject *obj)
{
    if (obj == NIL || obj == NULL)
        return NIL;
    if (LISP_TYPE(obj) != LISP_CONS)
        return NIL;
    return LISP_CDR(obj);
}

LispObject *lisp_set_car(LispObject *cell, LispObject *new_car)
{
    if (cell == NULL || cell == NIL || LISP_TYPE(cell) != LISP_CONS)
        return lisp_make_error("set-car: argument must be a cons cell");
    LISP_CAR(cell) = new_car;
    return cell;
}

LispObject *lisp_set_cdr(LispObject *cell, LispObject *new_cdr)
{
    if (cell == NULL || cell == NIL || LISP_TYPE(cell) != LISP_CONS)
        return lisp_make_error("set-cdr: argument must be a cons cell");
    LISP_CDR(cell) = new_cdr;
    return cell;
}

/* c*r combination helpers */
LispObject *lisp_caar(LispObject *obj)
{
    return lisp_car(lisp_car(obj));
}

LispObject *lisp_cadr(LispObject *obj)
{
    return lisp_car(lisp_cdr(obj));
}

LispObject *lisp_cdar(LispObject *obj)
{
    return lisp_cdr(lisp_car(obj));
}

LispObject *lisp_cddr(LispObject *obj)
{
    return lisp_cdr(lisp_cdr(obj));
}

LispObject *lisp_caddr(LispObject *obj)
{
    return lisp_car(lisp_cddr(obj));
}

LispObject *lisp_cadddr(LispObject *obj)
{
    return lisp_car(lisp_cdr(lisp_cddr(obj)));
}

/* Simple API */
static Environment *global_env = NULL;

/* Forward declaration for builtin registration */
void register_builtins(Environment *env);

/* Standard library macros (embedded to avoid file dependency) */
static const char *stdlib_code =
    ";; defun macro\n"
    "(defmacro defun (name params . body) `(define ,name (lambda ,params ,@body)))\n"
    "\n"
    ";; when - execute body when condition is true\n"
    "(defmacro when (condition . body)\n"
    "  \"Execute BODY when CONDITION is true.\"\n"
    "  `(if ,condition (progn ,@body) nil))\n"
    "\n"
    ";; unless - execute body when condition is false\n"
    "(defmacro unless (condition . body)\n"
    "  \"Execute BODY when CONDITION is false.\"\n"
    "  `(if ,condition nil (progn ,@body)))\n"
    "\n"
    ";; defvar - define variable only if unbound\n"
    ";; Usage: (defvar name) or (defvar name value) or (defvar name value docstring)\n"
    "(defmacro defvar (name . rest)\n"
    "  \"Define NAME as a variable. Only sets value if NAME is not already bound.\"\n"
    "  (let ((value (if (null? rest) nil (car rest)))\n"
    "        (docstring (if (or (null? rest) (null? (cdr rest))) nil (car (cdr rest)))))\n"
    "    `(progn\n"
    "       (unless (bound? ',name)\n"
    "         (define ,name ,value))\n"
    "       ,(if docstring\n"
    "            `(set-documentation! ',name ,docstring)\n"
    "            nil)\n"
    "       ',name)))\n"
    "\n"
    ";; defconst - define constant (always sets value)\n"
    ";; Usage: (defconst name value) or (defconst name value docstring)\n"
    "(defmacro defconst (name value . rest)\n"
    "  \"Define NAME as a constant. Always sets value (unlike defvar).\"\n"
    "  (let ((docstring (if (null? rest) nil (car rest))))\n"
    "    `(progn\n"
    "       (define ,name ,value)\n"
    "       ,(if docstring\n"
    "            `(set-documentation! ',name ,docstring)\n"
    "            nil)\n"
    "       ',name)))\n"
    "\n"
    ";; defalias - create alias for function\n"
    ";; Usage: (defalias alias target) or (defalias alias target docstring)\n"
    "(defmacro defalias (alias target . rest)\n"
    "  \"Define ALIAS as an alias for TARGET function.\"\n"
    "  (let ((docstring (if (null? rest) nil (car rest))))\n"
    "    `(progn\n"
    "       (define ,alias ,target)\n"
    "       ,(if docstring\n"
    "            `(set-documentation! ',alias ,docstring)\n"
    "            nil)\n"
    "       ',alias)))\n"
    "\n"
    ";; Short aliases\n"
    "(defalias doc documentation \"Shorthand for `documentation`.\")\n"
    "(defalias doc-set! set-documentation! \"Shorthand for `set-documentation!`.\")\n"
    "(defalias string-append concat \"Alias for `concat`.\")\n"
    "(defalias string-split split \"Alias for `split`.\")\n"
    "(defalias string-join join \"Alias for `join`.\")\n"
    "(defalias string-length length \"Alias for `length` (also works on lists and vectors).\")\n"
    "\n"
    ";; Package aliases (package- prefix for discoverability)\n"
    "(defalias package-set in-package \"Alias for `in-package`. Set the current package.\")\n"
    "(defalias package-current current-package \"Alias for `current-package`. Return current package name.\")\n"
    "(defalias package-list list-packages \"Alias for `list-packages`. Return list of all package names.\")\n";

/* Helper to load stdlib from embedded string */
static int load_stdlib(Environment *env)
{
    const char *input = stdlib_code;
    while (*input) {
        /* Skip whitespace and comments */
        while (*input && (*input == ' ' || *input == '\t' || *input == '\n' || *input == '\r')) {
            input++;
        }
        if (*input == ';') {
            while (*input && *input != '\n')
                input++;
            continue;
        }
        if (!*input)
            break;

        LispObject *expr = lisp_read(&input);
        if (expr == NULL)
            break;
        if (LISP_TYPE(expr) == LISP_ERROR) {
            return 0; /* Parse error */
        }

        LispObject *result = lisp_eval(expr, env);
        if (result != NULL && LISP_TYPE(result) == LISP_ERROR) {
            return 0; /* Eval error */
        }
    }
    return 1; /* Success */
}

Environment *lisp_init(void)
{
    if (global_env != NULL) {
        return global_env;
    }

    /* Initialize Boehm GC */
    GC_INIT();

    /* Initialize symbol intern table */
    symbol_table = lisp_make_hash_table();

    /* Pre-intern special form symbols */
#define INIT_SYM(c_name, lisp_name) c_name = lisp_intern(lisp_name);
    SPECIAL_FORMS(INIT_SYM)
#undef INIT_SYM

    /* Non-special-form pre-interned symbols */
    sym_unquote = lisp_intern("unquote");
    sym_unquote_splicing = lisp_intern("unquote-splicing");
    sym_else = lisp_intern("else");
    sym_optional = lisp_intern("&optional");
    sym_rest = lisp_intern("&rest");
    sym_error = lisp_intern("error");
    sym_package_ref = lisp_intern("package-ref");
    sym_star_package_star = lisp_intern("*package*");
    pkg_core = LISP_SYM_VAL(lisp_intern("core"));
    pkg_user = LISP_SYM_VAL(lisp_intern("user"));

    /* Create environment with builtins and stdlib */
    Environment *env = env_create(NULL);

    /* Set *package* to core for builtin and stdlib registration */
    env_define(env, LISP_SYM_VAL(sym_star_package_star), lisp_intern("core"), pkg_core);
    register_builtins(env);

    /* Load standard library (defun, defvar, defconst, defalias, aliases) */
    if (!load_stdlib(env)) {
        return NULL;
    }

    /* Switch to user package for user code */
    env_define(env, LISP_SYM_VAL(sym_star_package_star), lisp_intern("user"), pkg_core);

    global_env = env;
    return env;
}

LispObject *lisp_eval_string(const char *code, Environment *env)
{
    if (env == NULL) {
        env = global_env;
    }

    const char *input = code;
    LispObject *expr = lisp_read(&input);

    if (expr == NULL) {
        return NIL;
    }

    if (LISP_TYPE(expr) == LISP_ERROR) {
        return expr;
    }

    return lisp_eval(expr, env);
}

void lisp_cleanup(void)
{
    /* GC handles cleanup automatically */
    if (global_env != NULL) {
        env_free(global_env);
        global_env = NULL;
    }
}

/* Load file */
LispObject *lisp_load_file(const char *filename, Environment *env)
{
    /* Resolve filename via XDG data dirs for non-absolute paths */
    char resolved_path[4096];
    const char *path = file_resolve(filename, resolved_path, sizeof(resolved_path));

    /* Use binary mode to avoid ftell/fread size mismatch with CRLF translation */
    FILE *file = file_open(path, "rb");
    if (file == NULL) {
        return lisp_make_error("Cannot open file");
    }

    /* Read entire file */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = GC_malloc(size + 1);
    size_t actual_read = fread(buffer, 1, size, file);
    buffer[actual_read] = '\0';
    fclose(file);

    /* Evaluate all expressions */
    const char *input = buffer;
    LispObject *result = NIL;

    while (*input) {
        LispObject *expr = lisp_read(&input);
        if (expr == NULL)
            break;

        if (LISP_TYPE(expr) == LISP_ERROR) {
            return expr;
        }

        result = lisp_eval(expr, env);

        if (LISP_TYPE(result) == LISP_ERROR) {
            return result;
        }
    }

    return result;
}

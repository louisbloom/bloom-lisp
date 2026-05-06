#include "builtins_internal.h"

/* Check if a value needs constructor forms (not just lisp_print + quote) */
static int needs_constructor(LispObject *obj)
{
    if (obj == NULL || obj == NIL)
        return 0;
    switch (LISP_TYPE(obj)) {
    case LISP_LAMBDA:
    case LISP_MACRO:
    case LISP_BUILTIN:
    case LISP_HASH_TABLE:
        return 1;
    case LISP_CONS:
    {
        LispObject *cur = obj;
        while (cur != NIL && LISP_TYPE(cur) == LISP_CONS) {
            if (needs_constructor(lisp_car(cur)))
                return 1;
            cur = lisp_cdr(cur);
        }
        if (cur != NIL && needs_constructor(cur))
            return 1;
        return 0;
    }
    case LISP_VECTOR:
        for (size_t i = 0; i < LISP_VECTOR_SIZE(obj); i++) {
            if (needs_constructor(LISP_VECTOR_ITEMS(obj)[i]))
                return 1;
        }
        return 0;
    default:
        return 0;
    }
}

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
    switch (LISP_TYPE(val)) {
    case LISP_LAMBDA:
    {
        /* Check if already collected (pointer equality) */
        for (ExtractedLambda *e = *list; e != NULL; e = e->next) {
            if (e->obj == val)
                return;
        }
        ExtractedLambda *entry = GC_MALLOC(sizeof(ExtractedLambda));
        entry->obj = val;
        if (LISP_LAMBDA_NAME(val) && strncmp(LISP_LAMBDA_NAME(val), "lambda", 6) != 0) {
            snprintf(entry->name, sizeof(entry->name), "%s", LISP_LAMBDA_NAME(val));
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
        LispObject *body = LISP_LAMBDA_BODY(val);
        while (body != NIL && LISP_TYPE(body) == LISP_CONS) {
            collect_lambdas(lisp_car(body), list, counter, binding_name);
            body = lisp_cdr(body);
        }
        break;
    }
    case LISP_CONS:
    {
        LispObject *cur = val;
        while (cur != NIL && LISP_TYPE(cur) == LISP_CONS) {
            collect_lambdas(lisp_car(cur), list, counter, binding_name);
            cur = lisp_cdr(cur);
        }
        if (cur != NIL)
            collect_lambdas(cur, list, counter, binding_name);
        break;
    }
    case LISP_VECTOR:
        for (size_t i = 0; i < LISP_VECTOR_SIZE(val); i++) {
            collect_lambdas(LISP_VECTOR_ITEMS(val)[i], list, counter, binding_name);
        }
        break;
    case LISP_HASH_TABLE:
    {
        struct HashEntry **buckets = (struct HashEntry **)LISP_HT_BUCKETS(val);
        size_t bucket_count = LISP_HT_BUCKET_COUNT(val);
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
    fprintf(f, "(defun %s %s", name, lisp_print(LISP_LAMBDA_PARAMS(lambda)));
    if (LISP_LAMBDA_DOCSTRING(lambda)) {
        fprintf(f, "\n  ");
        write_escaped_string(f, LISP_LAMBDA_DOCSTRING(lambda));
    }
    LispObject *body = LISP_LAMBDA_BODY(lambda);
    while (body != NIL && LISP_TYPE(body) == LISP_CONS) {
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
    switch (LISP_TYPE(val)) {
    case LISP_LAMBDA:
    {
        /* In defun mode, emit extracted name instead of inline lambda */
        const char *extracted_name = extracted ? find_lambda_name(extracted, val) : NULL;
        if (extracted_name) {
            fprintf(f, "%s", extracted_name);
        } else {
            fprintf(f, "(lambda %s", lisp_print(LISP_LAMBDA_PARAMS(val)));
            /* Emit docstring if present */
            if (LISP_LAMBDA_DOCSTRING(val)) {
                fprintf(f, " ");
                write_escaped_string(f, LISP_LAMBDA_DOCSTRING(val));
            }
            LispObject *body = LISP_LAMBDA_BODY(val);
            while (body != NIL && LISP_TYPE(body) == LISP_CONS) {
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
        fprintf(f, "(lambda %s", lisp_print(LISP_MACRO_PARAMS(val)));
        if (LISP_MACRO_DOCSTRING(val)) {
            fprintf(f, " ");
            write_escaped_string(f, LISP_MACRO_DOCSTRING(val));
        }
        LispObject *body = LISP_MACRO_BODY(val);
        while (body != NIL && LISP_TYPE(body) == LISP_CONS) {
            fprintf(f, " %s", lisp_print(lisp_car(body)));
            body = lisp_cdr(body);
        }
        fprintf(f, ")");
        break;
    }
    case LISP_BUILTIN:
        /* Emit the builtin's name as a symbol reference */
        fprintf(f, "%s", LISP_BUILTIN_NAME(val));
        break;
    case LISP_HASH_TABLE:
    {
        fprintf(f, "(let ((ht (make-hash-table)))");
        struct HashEntry **buckets = (struct HashEntry **)LISP_HT_BUCKETS(val);
        size_t bucket_count = LISP_HT_BUCKET_COUNT(val);
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
            while (cur != NIL && LISP_TYPE(cur) == LISP_CONS) {
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
            for (size_t i = 0; i < LISP_VECTOR_SIZE(val); i++) {
                fprintf(f, " ");
                write_value_expr(f, LISP_VECTOR_ITEMS(val)[i], extracted);
            }
            fprintf(f, ")");
        } else {
            fprintf(f, "%s", lisp_print(val));
        }
        break;
    }
    case LISP_BOOLEAN:
        fprintf(f, "%s", LISP_BOOL_VAL(val) ? "#t" : "#f");
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
    while (rest != NIL && LISP_TYPE(rest) == LISP_CONS) {
        LispObject *arg = lisp_car(rest);
        if (LISP_TYPE(arg) == LISP_KEYWORD && strcmp(LISP_SYM_VAL(arg)->name, ":defun") == 0) {
            defun_mode = 1;
        } else if (LISP_TYPE(arg) == LISP_KEYWORD && strcmp(LISP_SYM_VAL(arg)->name, ":format") == 0) {
            format_mode = 1;
        } else if (LISP_TYPE(arg) == LISP_STRING) {
            target_pkg = LISP_SYM_VAL(lisp_intern(LISP_STR_VAL(arg)));
        } else if (LISP_TYPE(arg) == LISP_SYMBOL) {
            target_pkg = LISP_SYM_VAL(arg);
        }
        rest = lisp_cdr(rest);
    }
    if (target_pkg == NULL) {
        target_pkg = env_current_package(env);
    }

    if (LISP_TYPE(filename_obj) != LISP_STRING) {
        return lisp_make_error("package-save requires a string filename");
    }

    FILE *f = file_open(LISP_STR_VAL(filename_obj), "w");
    if (f == NULL) {
        char error[512];
        snprintf(error, sizeof(error), "package-save: cannot open '%s': %s", LISP_STR_VAL(filename_obj),
                 strerror(errno));
        return lisp_make_error(error);
    }

    fprintf(f, ";; Bloom Lisp package saved by package-save\n");
    fprintf(f, ";; Load with: (load \"%s\")\n", LISP_STR_VAL(filename_obj));
    fprintf(f, "(in-package \"%s\")\n\n", target_pkg->name);

    /* Collect bindings matching target package from all env frames */
    LispObject *bindings = NIL;
    Environment *e = env;
    while (e != NULL) {
        ENV_FOR_EACH_BINDING(e, binding)
        {
            if (binding->package == target_pkg) {
                /* Skip *package* itself */
                if (binding->symbol == LISP_SYM_VAL(sym_star_package_star))
                    continue;
                LispObject *pair = lisp_make_cons(lisp_make_string(binding->symbol->name), binding->value);
                bindings = lisp_make_cons(pair, bindings);
            }
        }
        e = e->parent;
    }

    /* Iterate in definition order */
    LispObject *cur = bindings;
    while (cur != NIL && LISP_TYPE(cur) == LISP_CONS) {
        LispObject *pair = lisp_car(cur);
        const char *name = LISP_STR_VAL(lisp_car(pair));
        LispObject *val = lisp_cdr(pair);

        /* Skip non-serializable types */
        if (val != NULL && (LISP_TYPE(val) == LISP_FILE_STREAM || LISP_TYPE(val) == LISP_STRING_PORT)) {
            fprintf(f, ";; Skipped %s (non-serializable %s)\n", name,
                    LISP_TYPE(val) == LISP_FILE_STREAM ? "file-stream" : "string-port");
            cur = lisp_cdr(cur);
            continue;
        }

        if (val != NULL && LISP_TYPE(val) == LISP_MACRO) {
            /* Emit defmacro form */
            fprintf(f, "(defmacro %s %s", name, lisp_print(LISP_MACRO_PARAMS(val)));
            if (LISP_MACRO_DOCSTRING(val)) {
                fprintf(f, "\n  ");
                write_escaped_string(f, LISP_MACRO_DOCSTRING(val));
            }
            LispObject *body = LISP_MACRO_BODY(val);
            while (body != NIL && LISP_TYPE(body) == LISP_CONS) {
                fprintf(f, "\n  %s", lisp_print(lisp_car(body)));
                body = lisp_cdr(body);
            }
            fprintf(f, ")\n\n");
        } else if (defun_mode && val != NULL && LISP_TYPE(val) == LISP_LAMBDA) {
            /* Top-level lambda: emit as defun directly */
            write_defun(f, name, val);
        } else if (defun_mode && val != NULL) {
            /* Extract nested lambdas, emit defuns, then define */
            ExtractedLambda *extracted = NULL;
            int counter = 0;
            collect_lambdas(val, &extracted, &counter, name);
            for (ExtractedLambda *el = extracted; el != NULL; el = el->next) {
                write_defun(f, el->name, el->obj);
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
        LispObject *fmt_func = env_lookup(env, LISP_SYM_VAL(fmt_sym));
        if (fmt_func == NULL) {
            return lisp_make_error(
                "package-save: :format requires lisp-fmt.lisp to be loaded first");
        }
        LispObject *call = lisp_make_cons(fmt_sym, lisp_make_cons(filename_obj, NIL));
        LispObject *result = lisp_eval(call, env);
        if (result != NULL && LISP_TYPE(result) == LISP_ERROR) {
            return result;
        }
        if (result != NULL && LISP_TYPE(result) == LISP_STRING) {
            FILE *f2 = file_open(LISP_STR_VAL(filename_obj), "w");
            if (f2 != NULL) {
                fputs(LISP_STR_VAL(result), f2);
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

    if (LISP_TYPE(arg) == LISP_STRING) {
        pkg_sym_obj = lisp_intern(LISP_STR_VAL(arg));
    } else if (LISP_TYPE(arg) == LISP_SYMBOL) {
        pkg_sym_obj = arg;
    } else {
        return lisp_make_error("in-package requires a string or symbol argument");
    }

    env_set(env, LISP_SYM_VAL(sym_star_package_star), pkg_sym_obj);
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

    if (LISP_TYPE(arg) == LISP_STRING) {
        target_pkg = LISP_SYM_VAL(lisp_intern(LISP_STR_VAL(arg)));
    } else if (LISP_TYPE(arg) == LISP_SYMBOL) {
        target_pkg = LISP_SYM_VAL(arg);
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
                while (cur != NIL && LISP_TYPE(cur) == LISP_CONS) {
                    if (LISP_TYPE(lisp_car(cur)) == LISP_SYMBOL &&
                        LISP_SYM_VAL(lisp_car(cur)) == binding->package) {
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

void register_packages_builtins(Environment *env)
{
    REGISTER("in-package", builtin_in_package);
    REGISTER("current-package", builtin_current_package);
    REGISTER("package-symbols", builtin_package_symbols);
    REGISTER("list-packages", builtin_list_packages);
    REGISTER("package-save", builtin_package_save);
}

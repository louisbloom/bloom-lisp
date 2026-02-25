#include "builtins_internal.h"

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

void register_printing_builtins(Environment *env)
{
    REGISTER("princ", builtin_princ);
    REGISTER("prin1", builtin_prin1);
    REGISTER("print", builtin_print_cl);
    REGISTER("format", builtin_format);
    REGISTER("terpri", builtin_terpri);
}

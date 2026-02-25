#include "builtins_internal.h"

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

void register_errors_builtins(Environment *env)
{
    REGISTER("error?", builtin_error_question);
    REGISTER("error-type", builtin_error_type);
    REGISTER("error-message", builtin_error_message);
    REGISTER("error-stack", builtin_error_stack);
    REGISTER("error-data", builtin_error_data);
    REGISTER("signal", builtin_signal);
    REGISTER("error", builtin_error);
}

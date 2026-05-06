#include "builtins_internal.h"

static LispObject *builtin_open_input_string(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("open-input-string");

    LispObject *str = lisp_car(args);
    if (LISP_TYPE(str) != LISP_STRING) {
        return lisp_make_error("open-input-string requires a string");
    }

    return lisp_make_string_port(LISP_STR_VAL(str));
}

static LispObject *builtin_port_peek_char(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-peek-char");

    LispObject *port = lisp_car(args);
    if (LISP_TYPE(port) != LISP_STRING_PORT) {
        return lisp_make_error("port-peek-char requires a string port");
    }

    if (LISP_STRING_PORT_BYTE_POS(port) >= LISP_STRING_PORT_BYTE_LEN(port)) {
        return NIL; /* EOF */
    }

    const char *ptr = LISP_STRING_PORT_BUFFER(port) + LISP_STRING_PORT_BYTE_POS(port);
    return lisp_make_char(utf8_get_codepoint(ptr));
}

static LispObject *builtin_port_read_char(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-read-char");

    LispObject *port = lisp_car(args);
    if (LISP_TYPE(port) != LISP_STRING_PORT) {
        return lisp_make_error("port-read-char requires a string port");
    }

    if (LISP_STRING_PORT_BYTE_POS(port) >= LISP_STRING_PORT_BYTE_LEN(port)) {
        return NIL; /* EOF */
    }

    const char *ptr = LISP_STRING_PORT_BUFFER(port) + LISP_STRING_PORT_BYTE_POS(port);
    unsigned int cp = utf8_get_codepoint(ptr);
    int bytes = utf8_char_bytes(ptr);
    LISP_STRING_PORT_BYTE_POS(port) += bytes;
    LISP_STRING_PORT_CHAR_POS(port)
    ++;
    return lisp_make_char(cp);
}

static LispObject *builtin_port_position(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-position");

    LispObject *port = lisp_car(args);
    if (LISP_TYPE(port) != LISP_STRING_PORT) {
        return lisp_make_error("port-position requires a string port");
    }

    return lisp_make_integer((long long)LISP_STRING_PORT_CHAR_POS(port));
}

static LispObject *builtin_port_source(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-source");

    LispObject *port = lisp_car(args);
    if (LISP_TYPE(port) != LISP_STRING_PORT) {
        return lisp_make_error("port-source requires a string port");
    }

    return lisp_make_string(LISP_STRING_PORT_BUFFER(port));
}

static LispObject *builtin_port_eof_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("port-eof?");

    LispObject *port = lisp_car(args);
    if (LISP_TYPE(port) != LISP_STRING_PORT) {
        return lisp_make_error("port-eof? requires a string port");
    }

    if (LISP_STRING_PORT_BYTE_POS(port) >= LISP_STRING_PORT_BYTE_LEN(port)) {
        return LISP_TRUE;
    }
    return NIL;
}

static LispObject *builtin_string_port_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("string-port?");

    LispObject *obj = lisp_car(args);
    return (LISP_TYPE(obj) == LISP_STRING_PORT) ? LISP_TRUE : NIL;
}

void register_string_ports_builtins(Environment *env)
{
    REGISTER("open-input-string", builtin_open_input_string);
    REGISTER("port-peek-char", builtin_port_peek_char);
    REGISTER("port-read-char", builtin_port_read_char);
    REGISTER("port-position", builtin_port_position);
    REGISTER("port-source", builtin_port_source);
    REGISTER("port-eof?", builtin_port_eof_question);
    REGISTER("string-port?", builtin_string_port_question);
}

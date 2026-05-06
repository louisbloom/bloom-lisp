/* lisp_value.h
 *
 * Accessor macros and inline helpers for LispObject values.
 *
 * This layer exists so that the underlying representation of LispObject
 * can evolve (box-out of fat union variants, low-bit pointer tagging
 * for fixnums/chars/nil/true) without requiring callers to change.
 *
 * The contract:
 *   - Code outside of lisp.c / eval.c MUST NOT touch obj->type or
 *     obj->value.X directly. Always go through LISP_TYPE(v),
 *     LISP_INT_VAL(v), LISP_*_VAL(v), LISP_CAR(v), LISP_CDR(v), etc.
 *   - Field-level macros (LISP_LAMBDA_NAME, LISP_ERROR_MESSAGE, ...)
 *     hide the difference between an in-union struct and a pointer
 *     to a separately-allocated info struct.
 *
 * In its current form (Stage 1) every macro expands to direct field
 * access; behavior is unchanged. Later stages reimplement these macros
 * to follow boxed-out pointers and decode tagged immediates.
 */

#ifndef LISP_VALUE_H
#define LISP_VALUE_H

/* lisp.h is the canonical include and pulls this file in last. */

/* ---- Type discrimination ---- */
#define LISP_TYPE(v) ((v)->type)

/* ---- Primitive value extractors ---- */
#define LISP_INT_VAL(v)  ((v)->value.integer)
#define LISP_NUM_VAL(v)  ((v)->value.number)
#define LISP_CHAR_VAL(v) ((v)->value.character)
#define LISP_BOOL_VAL(v) ((v)->value.boolean)

/* ---- Pointer-shaped value extractors ---- */
#define LISP_STR_VAL(v) ((v)->value.string)
#define LISP_SYM_VAL(v) ((v)->value.symbol)

/* ---- Cons accessors (lvalues) ---- */
#define LISP_CAR(v) ((v)->value.cons.car)
#define LISP_CDR(v) ((v)->value.cons.cdr)

/* ---- Builtin ---- */
#define LISP_BUILTIN_FUNC(v) ((v)->value.builtin.func)
#define LISP_BUILTIN_NAME(v) ((v)->value.builtin.name)

/* ---- Lambda fields (boxed out — pointer indirection) ---- */
#define LISP_LAMBDA_PARAMS(v)          ((v)->value.lambda->params)
#define LISP_LAMBDA_REQUIRED_PARAMS(v) ((v)->value.lambda->required_params)
#define LISP_LAMBDA_OPTIONAL_PARAMS(v) ((v)->value.lambda->optional_params)
#define LISP_LAMBDA_REST_PARAM(v)      ((v)->value.lambda->rest_param)
#define LISP_LAMBDA_REQUIRED_COUNT(v)  ((v)->value.lambda->required_count)
#define LISP_LAMBDA_OPTIONAL_COUNT(v)  ((v)->value.lambda->optional_count)
#define LISP_LAMBDA_BODY(v)            ((v)->value.lambda->body)
#define LISP_LAMBDA_CLOSURE(v)         ((v)->value.lambda->closure)
#define LISP_LAMBDA_NAME(v)            ((v)->value.lambda->name)
#define LISP_LAMBDA_DOCSTRING(v)       ((v)->value.lambda->docstring)

/* ---- Macro fields (boxed out — pointer indirection) ---- */
#define LISP_MACRO_PARAMS(v)    ((v)->value.macro->params)
#define LISP_MACRO_BODY(v)      ((v)->value.macro->body)
#define LISP_MACRO_CLOSURE(v)   ((v)->value.macro->closure)
#define LISP_MACRO_NAME(v)      ((v)->value.macro->name)
#define LISP_MACRO_DOCSTRING(v) ((v)->value.macro->docstring)

/* ---- Error fields (boxed out — pointer indirection) ---- */
#define LISP_ERROR_TYPE(v)        ((v)->value.error_with_stack->error_type)
#define LISP_ERROR_MESSAGE(v)     ((v)->value.error_with_stack->message)
#define LISP_ERROR_DATA(v)        ((v)->value.error_with_stack->data)
#define LISP_ERROR_STACK_TRACE(v) ((v)->value.error_with_stack->stack_trace)
#define LISP_ERROR_CAUGHT(v)      ((v)->value.error_with_stack->caught)

/* ---- String port fields (boxed out — pointer indirection) ---- */
#define LISP_STRING_PORT_BUFFER(v)   ((v)->value.string_port->buffer)
#define LISP_STRING_PORT_BYTE_LEN(v) ((v)->value.string_port->byte_len)
#define LISP_STRING_PORT_CHAR_LEN(v) ((v)->value.string_port->char_len)
#define LISP_STRING_PORT_BYTE_POS(v) ((v)->value.string_port->byte_pos)
#define LISP_STRING_PORT_CHAR_POS(v) ((v)->value.string_port->char_pos)

/* ---- File ---- */
#define LISP_FILE_FP(v)  ((v)->value.file.fp)
#define LISP_FILE_EOL(v) ((v)->value.file.eol)

/* ---- Vector (boxed out — pointer indirection) ---- */
#define LISP_VECTOR_ITEMS(v)    ((v)->value.vector->items)
#define LISP_VECTOR_SIZE(v)     ((v)->value.vector->size)
#define LISP_VECTOR_CAPACITY(v) ((v)->value.vector->capacity)

/* ---- Hash table (boxed out — pointer indirection) ---- */
#define LISP_HT_BUCKETS(v)      ((v)->value.hash_table->buckets)
#define LISP_HT_BUCKET_COUNT(v) ((v)->value.hash_table->bucket_count)
#define LISP_HT_ENTRY_COUNT(v)  ((v)->value.hash_table->entry_count)
#define LISP_HT_CAPACITY(v)     ((v)->value.hash_table->capacity)

/* ---- Tail call ---- */
#define LISP_TAIL_CALL_FUNC(v) ((v)->value.tail_call.func)
#define LISP_TAIL_CALL_ARGS(v) ((v)->value.tail_call.args)

/* ---- Regex ---- */
#define LISP_REGEX_CODE(v) ((v)->value.regex.code)

/* ---- Public setter for cons cdr (replaces (*tail)->value.cons.cdr = ...) ---- */
LispObject *lisp_set_car(LispObject *cell, LispObject *new_car);
LispObject *lisp_set_cdr(LispObject *cell, LispObject *new_cdr);

#endif /* LISP_VALUE_H */

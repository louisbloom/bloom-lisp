/* lisp_value.h
 *
 * Accessor macros and inline helpers for LispObject values.
 *
 * The LispObject representation has two complications:
 *
 *   1. Several fat union variants (lambda, macro, error_with_stack,
 *      string_port, vector, hash_table) are boxed out into separately
 *      allocated *Info structs, so sizeof(LispObject) stays small (24 B).
 *      Their field macros (LISP_LAMBDA_NAME, LISP_ERROR_MESSAGE, ...)
 *      follow the boxed pointer for you.
 *
 *   2. Fixnums, chars, nil, and #t are encoded directly in the
 *      LispObject* value via low-bit tagging — no heap allocation. The
 *      LISP_TYPE / LISP_INT_VAL / LISP_CHAR_VAL / LISP_BOOL_VAL accessors
 *      decode the tag bits transparently.
 *
 * The contract for callers:
 *   - NEVER dereference a LispObject* directly (no obj->type, no
 *     obj->value.X). A tagged value (fixnum, char, nil, #t) is not a
 *     valid pointer to dereference; it'll SIGSEGV.
 *   - Always go through the LISP_* macros. They handle tag bits, NULL,
 *     and the boxed-out indirection for you.
 *   - Inside lisp.c constructors, direct (obj->type = ..., obj->value.X
 *     = ...) initialization on a freshly GC_malloc'd heap object IS
 *     allowed — that's where the abstraction is built.
 */

#ifndef LISP_VALUE_H
#define LISP_VALUE_H

#include <stdint.h>

/* lisp.h is the canonical include and pulls this file in last. */

/* ---- Tagged pointer scheme (low 3 bits) ---------------------------------
 *   000  heap pointer   8-byte-aligned LispObject *  (real heap object)
 *   001  fixnum         (int64 << 3) | 1            (61-bit signed)
 *   010  char           (uint32 << 3) | 2           (21-bit unicode codepoint)
 *   011  singleton      NIL = 0x3, LISP_TRUE = 0xB
 *   100..111            reserved (future tags)
 *
 * Heap pointers stay as real pointers so Boehm GC's conservative scanner
 * recognizes them. Tagged immediates have low bits != 0 so they're outside
 * any heap region; if a fixnum coincidentally falls inside one GC may
 * conservatively keep that block alive (false retention only, never UAF).
 */
#define LISP_TAG_MASK      0x7
#define LISP_TAG_HEAP      0x0
#define LISP_TAG_FIXNUM    0x1
#define LISP_TAG_CHAR      0x2
#define LISP_TAG_SINGLETON 0x3

/* Singletons — compile-time constants, no heap object behind them. */
#undef NIL
#undef LISP_TRUE
#define NIL       ((LispObject *)(uintptr_t)0x3ULL)
#define LISP_TRUE ((LispObject *)(uintptr_t)0xBULL)

/* ---- Type discrimination ----------------------------------------------- */
static inline LispType lisp_type_inline(LispObject *v)
{
    uintptr_t bits = (uintptr_t)v;
    switch (bits & LISP_TAG_MASK) {
    case LISP_TAG_FIXNUM:
        return LISP_INTEGER;
    case LISP_TAG_CHAR:
        return LISP_CHAR;
    case LISP_TAG_SINGLETON:
        if (v == LISP_TRUE)
            return LISP_BOOLEAN;
        return LISP_NIL; /* NIL or unknown singleton */
    case LISP_TAG_HEAP:
        if (v == NULL)
            return LISP_NIL;
        return v->type;
    default:
        return LISP_NIL; /* reserved tags */
    }
}
#define LISP_TYPE(v) lisp_type_inline(v)

/* ---- Primitive value extractors ---------------------------------------- */
static inline long long lisp_int_val_inline(LispObject *v)
{
    uintptr_t bits = (uintptr_t)v;
    if ((bits & LISP_TAG_MASK) == LISP_TAG_FIXNUM)
        return (long long)((intptr_t)bits >> 3);
    return v->value.integer; /* big int, heap-allocated */
}
#define LISP_INT_VAL(v) lisp_int_val_inline(v)

#define LISP_NUM_VAL(v)  ((v)->value.number) /* doubles still heap */
#define LISP_CHAR_VAL(v) ((uint32_t)((uintptr_t)(v) >> 3))
#define LISP_BOOL_VAL(v) ((v) == LISP_TRUE)

/* ---- Pointer-shaped value extractors (heap-only) ----------------------- */
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

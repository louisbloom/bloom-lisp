/* test_sf_kind.c - Tests for lisp_sf_kind() special form classification */

#include "../include/lisp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_EQ(actual, expected, msg)                       \
    do {                                                       \
        tests_run++;                                           \
        if ((actual) == (expected)) {                          \
            tests_passed++;                                    \
        } else {                                               \
            fprintf(stderr, "FAIL: %s: expected %d, got %d\n", \
                    msg, (int)(expected), (int)(actual));      \
        }                                                      \
    } while (0)

#define ASSERT_NE(actual, not_expected, msg)                \
    do {                                                    \
        tests_run++;                                        \
        if ((actual) != (not_expected)) {                   \
            tests_passed++;                                 \
        } else {                                            \
            fprintf(stderr, "FAIL: %s: should not be %d\n", \
                    msg, (int)(not_expected));              \
        }                                                   \
    } while (0)

int main(void)
{
    Environment *env = lisp_init();

    /* ---- quote / quasiquote → SF_KIND_QUOTE ---- */
    ASSERT_EQ(lisp_sf_kind(sym_quote), SF_KIND_QUOTE,
              "quote should be SF_KIND_QUOTE");
    ASSERT_EQ(lisp_sf_kind(sym_quasiquote), SF_KIND_QUOTE,
              "quasiquote should be SF_KIND_QUOTE");

    /* ---- define / set! / lambda → SF_KIND_DEFINE ---- */
    ASSERT_EQ(lisp_sf_kind(sym_define), SF_KIND_DEFINE,
              "define should be SF_KIND_DEFINE");
    ASSERT_EQ(lisp_sf_kind(sym_set), SF_KIND_DEFINE,
              "set! should be SF_KIND_DEFINE");
    ASSERT_EQ(lisp_sf_kind(sym_lambda), SF_KIND_DEFINE,
              "lambda should be SF_KIND_DEFINE");

    /* ---- defmacro → SF_KIND_MACRO_DEF ---- */
    ASSERT_EQ(lisp_sf_kind(sym_defmacro), SF_KIND_MACRO_DEF,
              "defmacro should be SF_KIND_MACRO_DEF");

    /* ---- if / cond / case / and / or / do → SF_KIND_CONTROL ---- */
    ASSERT_EQ(lisp_sf_kind(sym_if), SF_KIND_CONTROL,
              "if should be SF_KIND_CONTROL");
    ASSERT_EQ(lisp_sf_kind(sym_cond), SF_KIND_CONTROL,
              "cond should be SF_KIND_CONTROL");
    ASSERT_EQ(lisp_sf_kind(sym_case), SF_KIND_CONTROL,
              "case should be SF_KIND_CONTROL");
    ASSERT_EQ(lisp_sf_kind(sym_and), SF_KIND_CONTROL,
              "and should be SF_KIND_CONTROL");
    ASSERT_EQ(lisp_sf_kind(sym_or), SF_KIND_CONTROL,
              "or should be SF_KIND_CONTROL");
    ASSERT_EQ(lisp_sf_kind(sym_do), SF_KIND_CONTROL,
              "do should be SF_KIND_CONTROL");

    /* ---- let / let* → SF_KIND_BINDING ---- */
    ASSERT_EQ(lisp_sf_kind(sym_let), SF_KIND_BINDING,
              "let should be SF_KIND_BINDING");
    ASSERT_EQ(lisp_sf_kind(sym_let_star), SF_KIND_BINDING,
              "let* should be SF_KIND_BINDING");

    /* ---- progn → SF_KIND_BODY ---- */
    ASSERT_EQ(lisp_sf_kind(sym_progn), SF_KIND_BODY,
              "progn should be SF_KIND_BODY");

    /* ---- condition-case / unwind-protect → SF_KIND_EXCEPTION ---- */
    ASSERT_EQ(lisp_sf_kind(sym_condition_case), SF_KIND_EXCEPTION,
              "condition-case should be SF_KIND_EXCEPTION");
    ASSERT_EQ(lisp_sf_kind(sym_unwind_protect), SF_KIND_EXCEPTION,
              "unwind-protect should be SF_KIND_EXCEPTION");

    /* ---- non-special-form symbol → -1 ---- */
    LispObject *non_sf = lisp_intern("car");
    ASSERT_EQ(lisp_sf_kind(non_sf), -1,
              "car should not be a special form");

    /* ---- freshly interned unknown symbol → -1 ---- */
    LispObject *unknown = lisp_intern("xyz-unknown-symbol-12345");
    ASSERT_EQ(lisp_sf_kind(unknown), -1,
              "unknown symbol should not be a special form");

    /* ---- SF_COUNT matches number of special forms ---- */
    ASSERT_EQ((int)lisp_sf_count(), 17,
              "SF_COUNT should be 17");

    lisp_cleanup();

    printf("%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}

/* test_api.c - Public API contract tests */

#include "flare_testkit.h"
#include <bloom-lisp/highlight.h>
#include <string.h>

static void test_result_free_null_safe(void)
{
    BflareResult r = { 0 };
    bflare_result_free(r);
}

static void test_lexer_free_null_safe(void)
{
    bflare_lexer_free(NULL);
}

static void test_style_free_null_safe(void)
{
    bflare_style_free(NULL);
}

static void test_formatter_free_null_safe(void)
{
    bflare_formatter_free(NULL);
}

static void test_highlight_defaults(void)
{
    BflareResult r = bflare_highlight("(+ 1 2)", 7, NULL, NULL, NULL);
    ASSERT_NOT_NULL(r.data);
    ASSERT_TRUE(r.length > 0);
    bflare_result_free(r);
}

static void test_highlight_empty_input(void)
{
    BflareResult r = bflare_highlight("", 0, NULL, NULL, NULL);
    ASSERT_NOT_NULL(r.data);
    bflare_result_free(r);
}

static void test_highlight_result_standalone(void)
{
    BflareResult r = bflare_highlight("(if #t 1 0)", 11, NULL, NULL, NULL);
    ASSERT_TRUE(strstr(r.data, "\033[") != NULL);
    bflare_result_free(r);
}

static void test_highlight_ends_with_sgr_reset(void)
{
    BflareResult r = bflare_highlight("(+ 1 2)", 7, NULL, NULL, NULL);
    size_t len = r.length;
    ASSERT_TRUE(len >= 4);
    ASSERT_STR_EQ(r.data + len - 4, "\033[0m");
    bflare_result_free(r);
}

int main(void)
{
    RUN_TEST(test_result_free_null_safe);
    RUN_TEST(test_lexer_free_null_safe);
    RUN_TEST(test_style_free_null_safe);
    RUN_TEST(test_formatter_free_null_safe);
    RUN_TEST(test_highlight_defaults);
    RUN_TEST(test_highlight_empty_input);
    RUN_TEST(test_highlight_result_standalone);
    RUN_TEST(test_highlight_ends_with_sgr_reset);
    TEST_SUMMARY();
}

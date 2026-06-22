/* test_style.c - Style hierarchy and built-in style tests */

#include "flare_testkit.h"
#include <bloom-lisp/highlight.h>

static void test_style_fallback_to_category(void)
{
    BflareStyle *s = bflare_style_new();
    BflareStyleEntry kw_entry = { .fg_r = 255, .fg_g = 0, .fg_b = 0 };
    bflare_style_set(s, HL_KEYWORD, &kw_entry);
    BflareStyleEntry result = bflare_style_get(s, HL_KEYWORD_SPECIAL_FORM);
    ASSERT_EQ(result.fg_r, 255);
    ASSERT_EQ(result.fg_g, 0);
    bflare_style_free(s);
}

static void test_style_specific_overrides_category(void)
{
    BflareStyle *s = bflare_style_new();
    BflareStyleEntry kw_entry = { .fg_r = 255, .fg_g = 0, .fg_b = 0 };
    BflareStyleEntry sf_entry = { .fg_r = 0, .fg_g = 255, .fg_b = 0 };
    bflare_style_set(s, HL_KEYWORD, &kw_entry);
    bflare_style_set(s, HL_KEYWORD_SPECIAL_FORM, &sf_entry);
    BflareStyleEntry result = bflare_style_get(s, HL_KEYWORD_SPECIAL_FORM);
    ASSERT_EQ(result.fg_r, 0);
    ASSERT_EQ(result.fg_g, 255);
    bflare_style_free(s);
}

static void test_style_fallback_to_text(void)
{
    BflareStyle *s = bflare_style_new();
    BflareStyleEntry text_entry = { .fg_r = 200, .fg_g = 200, .fg_b = 200 };
    bflare_style_set(s, HL_TEXT, &text_entry);
    BflareStyleEntry result = bflare_style_get(s, HL_PUNCT_DOT);
    ASSERT_EQ(result.fg_r, 200);
    bflare_style_free(s);
}

static void test_style_built_in_not_null(void)
{
    BflareStyle *s = bflare_style_monokai();
    ASSERT_NOT_NULL(s);
    bflare_style_free(s);
}

static void test_style_monokai_has_text_entry(void)
{
    BflareStyle *s = bflare_style_monokai();
    BflareStyleEntry e = bflare_style_get(s, HL_TEXT);
    ASSERT_TRUE(e.fg_r > 0 || e.fg_g > 0 || e.fg_b > 0);
    bflare_style_free(s);
}

static void test_style_monokai_keyword_is_colored(void)
{
    BflareStyle *s = bflare_style_monokai();
    BflareStyleEntry kw = bflare_style_get(s, HL_KEYWORD);
    BflareStyleEntry txt = bflare_style_get(s, HL_TEXT);
    ASSERT_TRUE(kw.fg_r != txt.fg_r || kw.fg_g != txt.fg_g || kw.fg_b != txt.fg_b);
    bflare_style_free(s);
}

static void test_style_monokai_all_standard_types(void)
{
    BflareStyle *s = bflare_style_monokai();
    int types[] = {
        HL_TEXT, HL_KEYWORD, HL_KEYWORD_SPECIAL_FORM, HL_KEYWORD_DEFINE,
        HL_NAME, HL_NAME_FUNCTION, HL_NAME_BUILTIN, HL_NAME_VARIABLE,
        HL_LITERAL, HL_LITERAL_STRING, HL_LITERAL_NUMBER,
        HL_OPERATOR, HL_PUNCTUATION, HL_COMMENT, HL_ERROR
    };
    for (int i = 0; i < (int)(sizeof(types) / sizeof(types[0])); i++) {
        BflareStyleEntry e = bflare_style_get(s, types[i]);
        (void)e;
    }
    bflare_style_free(s);
}

static void test_style_custom_override(void)
{
    BflareStyle *s = bflare_style_monokai();
    BflareStyleEntry custom = { .fg_r = 0, .fg_g = 255, .fg_b = 0 };
    bflare_style_set(s, HL_LITERAL_STRING, &custom);
    BflareStyleEntry result = bflare_style_get(s, HL_LITERAL_STRING);
    ASSERT_EQ(result.fg_r, 0);
    ASSERT_EQ(result.fg_g, 255);
    bflare_style_free(s);
}

int main(void)
{
    RUN_TEST(test_style_fallback_to_category);
    RUN_TEST(test_style_specific_overrides_category);
    RUN_TEST(test_style_fallback_to_text);
    RUN_TEST(test_style_built_in_not_null);
    RUN_TEST(test_style_monokai_has_text_entry);
    RUN_TEST(test_style_monokai_keyword_is_colored);
    RUN_TEST(test_style_monokai_all_standard_types);
    RUN_TEST(test_style_custom_override);
    TEST_SUMMARY();
}

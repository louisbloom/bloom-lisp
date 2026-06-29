/* test_token_type.c - Token type hierarchy tests */

#include "flare_testkit.h"
#include <ditty/highlight.h>

static void test_token_category_text(void)
{
    ASSERT_EQ(flare_token_category(HL_TEXT), 0);
}

static void test_token_category_keyword(void)
{
    ASSERT_EQ(flare_token_category(HL_KEYWORD_SPECIAL_FORM), 1000);
}

static void test_token_subcategory_kw(void)
{
    ASSERT_EQ(flare_token_subcategory(HL_KEYWORD_SPECIAL_FORM), 1010);
}

static void test_token_subcategory_literal(void)
{
    ASSERT_EQ(flare_token_subcategory(HL_LITERAL_STRING_ESCAPE), 3010);
}

static void test_token_ranges(void)
{
    int types[] = {
        HL_TEXT, HL_KEYWORD, HL_KEYWORD_SPECIAL_FORM, HL_KEYWORD_DEFINE,
        HL_NAME, HL_NAME_FUNCTION, HL_NAME_BUILTIN, HL_NAME_VARIABLE,
        HL_LITERAL, HL_LITERAL_STRING, HL_LITERAL_NUMBER,
        HL_OPERATOR, HL_OPERATOR_QUOTE,
        HL_PUNCTUATION, HL_PUNCT_OPEN_PAREN,
        HL_COMMENT, HL_COMMENT_LINE,
        HL_ERROR, HL_ERROR_UNCLOSED_STRING
    };
    int valid_cats[] = { 0, 1000, 2000, 3000, 4000, 5000, 6000, 7000 };
    for (int i = 0; i < (int)(sizeof(types) / sizeof(types[0])); i++) {
        int cat = flare_token_category(types[i]);
        int found = 0;
        for (int j = 0; j < (int)(sizeof(valid_cats) / sizeof(valid_cats[0])); j++) {
            if (cat == valid_cats[j]) {
                found = 1;
                break;
            }
        }
        ASSERT_TRUE(found);
    }
}

int main(void)
{
    RUN_TEST(test_token_category_text);
    RUN_TEST(test_token_category_keyword);
    RUN_TEST(test_token_subcategory_kw);
    RUN_TEST(test_token_subcategory_literal);
    RUN_TEST(test_token_ranges);
    TEST_SUMMARY();
}

/* test_lexer.c - Lexer tokenization tests */

#include "flare_testkit.h"
#include "lisp.h"
#include <bloom-lisp/highlight.h>

static Environment *env;

static void test_lexer_empty_input(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "", 0, &count);
    ASSERT_EQ(count, 0);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_whitespace_only(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "   \n\t  ", 7, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_TEXT);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_line_comment(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "; hello", 7, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_COMMENT_LINE);
    ASSERT_EQ(tokens[0].offset, 0);
    ASSERT_EQ(tokens[0].length, 7);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_string_double_quoted(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "\"hello\"", 7, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_STRING);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_string_with_escape(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "\"a\\nb\"", 6, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_STRING);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_unclosed_string(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "\"hello", 6, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_ERROR_UNCLOSED_STRING);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_open_paren(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "(", 1, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_PUNCT_OPEN_PAREN);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_close_paren(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, ")", 1, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_PUNCT_CLOSE_PAREN);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_quote(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "'foo", 4, &count);
    ASSERT_EQ(count, 2);
    ASSERT_EQ(tokens[0].type, HL_OPERATOR_QUOTE);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_backquote(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "`(list ,a ,@b)", 14, &count);
    ASSERT_EQ(tokens[0].type, HL_OPERATOR_BACKQUOTE);
    int found_splice = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_OPERATOR_UNQUOTE_SPLICING)
            found_splice = 1;
    ASSERT_TRUE(found_splice);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_integer(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "42", 2, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_NUMBER);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_float(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "3.14", 4, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_NUMBER);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_negative_number(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "-7", 2, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_NUMBER);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_character_literal(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "#\\space", 7, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_CHARACTER);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_boolean_true(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "#t", 2, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_BOOLEAN);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_boolean_false(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "#f", 2, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_NIL);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_nil(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "nil", 3, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_LITERAL_NIL);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_keyword_arg(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, ":foo", 4, &count);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(tokens[0].type, HL_NAME_KEYWORD_ARG);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_vector_literal(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "#(1 2 3)", 8, &count);
    ASSERT_TRUE(count >= 2);
    ASSERT_EQ(tokens[0].type, HL_PUNCT_HASH);
    ASSERT_EQ(tokens[1].type, HL_PUNCT_OPEN_PAREN);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_dotted_pair(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "(a . b)", 7, &count);
    int dot_found = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_PUNCT_DOT)
            dot_found = 1;
    ASSERT_TRUE(dot_found);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_env_special_form(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "if", 2, &count);
    ASSERT_EQ(tokens[0].type, HL_KEYWORD_CONTROL);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_partial_unclosed_paren(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    size_t count = 0;
    BflareToken *tokens = bflare_lex(lex, "(+ 1", 4, &count);
    int open_found = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_PUNCT_OPEN_PAREN)
            open_found = 1;
    ASSERT_TRUE(open_found);
    free(tokens);
    bflare_lexer_free(lex);
}

static void test_lexer_fuzz_no_crash(void)
{
    BflareLexer *lex = bflare_lexer_bloom_lisp(env);
    const char *edge_cases[] = {
        "", " ", "\n", "\t", "(", ")", "\"", "'", "`", ",",
        ",@", "#", "#\\", "#(", "#t", "#f", "nil", ";;;",
        "\"\"\"", "\\n", "(.", "( )", "(((", ")))",
        NULL
    };
    for (int i = 0; edge_cases[i]; i++) {
        size_t count;
        BflareToken *tokens = bflare_lex(lex, edge_cases[i],
                                         strlen(edge_cases[i]), &count);
        free(tokens);
    }
    bflare_lexer_free(lex);
}

int main(void)
{
    env = lisp_init();
    RUN_TEST(test_lexer_empty_input);
    RUN_TEST(test_lexer_whitespace_only);
    RUN_TEST(test_lexer_line_comment);
    RUN_TEST(test_lexer_string_double_quoted);
    RUN_TEST(test_lexer_string_with_escape);
    RUN_TEST(test_lexer_unclosed_string);
    RUN_TEST(test_lexer_open_paren);
    RUN_TEST(test_lexer_close_paren);
    RUN_TEST(test_lexer_quote);
    RUN_TEST(test_lexer_backquote);
    RUN_TEST(test_lexer_integer);
    RUN_TEST(test_lexer_float);
    RUN_TEST(test_lexer_negative_number);
    RUN_TEST(test_lexer_character_literal);
    RUN_TEST(test_lexer_boolean_true);
    RUN_TEST(test_lexer_boolean_false);
    RUN_TEST(test_lexer_nil);
    RUN_TEST(test_lexer_keyword_arg);
    RUN_TEST(test_lexer_vector_literal);
    RUN_TEST(test_lexer_dotted_pair);
    RUN_TEST(test_lexer_env_special_form);
    RUN_TEST(test_lexer_partial_unclosed_paren);
    RUN_TEST(test_lexer_fuzz_no_crash);
    lisp_cleanup();
    TEST_SUMMARY();
}

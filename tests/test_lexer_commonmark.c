/* test_lexer_commonmark.c - CommonMark lexer tokenization tests */

#include "flare_testkit.h"
#include "lisp.h"
#include <bloom-lisp/highlight.h>

static Environment *env;

/* ----- Block structure --------------------------------------------------- */

static void test_atx_heading(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "# Hello\n", 8, &count);
    ASSERT_TRUE(count >= 3);
    ASSERT_EQ(tokens[0].type, HL_MARKUP_HEADING_MARKER);
    ASSERT_EQ(tokens[0].offset, 0);
    ASSERT_EQ(tokens[0].length, 1);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_atx_heading_level2(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "## World\n", 9, &count);
    ASSERT_TRUE(count >= 3);
    ASSERT_EQ(tokens[0].type, HL_MARKUP_HEADING_MARKER);
    ASSERT_EQ(tokens[0].length, 2);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_thematic_break(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "---\n", 4, &count);
    ASSERT_EQ(count, 2);
    ASSERT_EQ(tokens[0].type, HL_MARKUP_THEMATIC_BREAK);
    ASSERT_EQ(tokens[0].offset, 0);
    ASSERT_EQ(tokens[0].length, 3);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_setext_heading(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "Title\n===\n", 10, &count);
    int found_setext = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_SETEXT_UNDERLINE)
            found_setext = 1;
    ASSERT_TRUE(found_setext);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_block_quote(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "> quote\n", 8, &count);
    int found_marker = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_BLOCKQUOTE_MARKER)
            found_marker = 1;
    ASSERT_TRUE(found_marker);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_bullet_list(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "- item\n", 7, &count);
    int found_marker = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_LIST_MARKER)
            found_marker = 1;
    ASSERT_TRUE(found_marker);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_ordered_list(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "1. first\n", 9, &count);
    int found_marker = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_LIST_MARKER)
            found_marker = 1;
    ASSERT_TRUE(found_marker);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_indented_code(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "    code\n", 9, &count);
    int found_code = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INDENTED_CODE)
            found_code = 1;
    ASSERT_TRUE(found_code);
    free(tokens);
    flare_lexer_free(lex);
}

/* ----- Fenced code blocks ----------------------------------------------- */

static void test_fenced_code_plain(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "```\nhello\n```\n", 14, &count);
    int found_open = 0, found_close = 0;
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type == HL_MARKUP_FENCED_OPEN)
            found_open = 1;
        if (tokens[i].type == HL_MARKUP_FENCED_CLOSE)
            found_close = 1;
    }
    ASSERT_TRUE(found_open);
    ASSERT_TRUE(found_close);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_fenced_code_lisp_suppresses_fences(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "```lisp\n(+ 1 2)\n```\n", 20, &count);
    ASSERT_TRUE(count > 0);
    int found_open = 0, found_close = 0;
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type == HL_MARKUP_FENCED_OPEN)
            found_open = 1;
        if (tokens[i].type == HL_MARKUP_FENCED_CLOSE)
            found_close = 1;
    }
    ASSERT_EQ(found_open, 0);
    ASSERT_EQ(found_close, 0);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_fenced_code_lisp_sublexes_content(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "```lisp\n(+ 1 2)\n```\n", 20, &count);
    ASSERT_TRUE(count > 0);
    int found_paren = 0, found_plus = 0;
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type == HL_PUNCT_OPEN_PAREN)
            found_paren = 1;
        if (tokens[i].type == HL_NAME_BUILTIN)
            found_plus = 1;
    }
    ASSERT_TRUE(found_paren);
    ASSERT_TRUE(found_plus);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_fenced_code_bloom_lisp_alias(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "```bloom-lisp\n(+ 1 2)\n```\n", 26, &count);
    int found_open = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_FENCED_OPEN)
            found_open = 1;
    ASSERT_EQ(found_open, 0);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_fenced_code_bloom_alias(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "```bloom\n(+ 1 2)\n```\n", 21, &count);
    int found_open = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_FENCED_OPEN)
            found_open = 1;
    ASSERT_EQ(found_open, 0);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_fenced_code_tilde(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "~~~\nplain\n~~~\n", 14, &count);
    int found_open = 0, found_close = 0;
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type == HL_MARKUP_FENCED_OPEN)
            found_open = 1;
        if (tokens[i].type == HL_MARKUP_FENCED_CLOSE)
            found_close = 1;
    }
    ASSERT_TRUE(found_open);
    ASSERT_TRUE(found_close);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_fenced_code_info_string(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "```python\nprint()\n```\n", 22, &count);
    int found_info = 0, found_open = 0, found_close = 0;
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type == HL_MARKUP_FENCED_INFO)
            found_info = 1;
        if (tokens[i].type == HL_MARKUP_FENCED_OPEN)
            found_open = 1;
        if (tokens[i].type == HL_MARKUP_FENCED_CLOSE)
            found_close = 1;
    }
    ASSERT_TRUE(found_info);
    ASSERT_TRUE(found_open);
    ASSERT_TRUE(found_close);
    free(tokens);
    flare_lexer_free(lex);
}

/* ----- Inline constructs ------------------------------------------------ */

static void test_inline_emphasis(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "hello *world* end\n", 18, &count);
    int found_emph = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_EMPHASIS)
            found_emph = 1;
    ASSERT_TRUE(found_emph);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_inline_strong(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "hello **world** end\n", 20, &count);
    int found_strong = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_STRONG)
            found_strong = 1;
    ASSERT_TRUE(found_strong);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_inline_code_span(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "use `car` here\n", 15, &count);
    int found_code = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_CODE)
            found_code = 1;
    ASSERT_TRUE(found_code);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_inline_link(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "click [here](url)\n", 18, &count);
    int found_link = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_LINK)
            found_link = 1;
    ASSERT_TRUE(found_link);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_inline_image(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "see ![alt](img)\n", 16, &count);
    int found_img = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_IMAGE)
            found_img = 1;
    ASSERT_TRUE(found_img);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_inline_escape(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "not \\*bold\\*\n", 14, &count);
    int found_escape = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_ESCAPE)
            found_escape = 1;
    ASSERT_TRUE(found_escape);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_inline_entity(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "&amp; &#123;\n", 13, &count);
    int found_entity = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_ENTITY)
            found_entity = 1;
    ASSERT_TRUE(found_entity);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_hard_break_spaces(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "line1  \nline2\n", 14, &count);
    int found_break = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_BREAK)
            found_break = 1;
    ASSERT_TRUE(found_break);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_hard_break_backslash(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "line1\\\nline2\n", 13, &count);
    int found_break = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_BREAK)
            found_break = 1;
    ASSERT_TRUE(found_break);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_autolink(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "<https://example.com>\n", 22, &count);
    int found_autolink = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == HL_MARKUP_INLINE_AUTOLINK)
            found_autolink = 1;
    ASSERT_TRUE(found_autolink);
    free(tokens);
    flare_lexer_free(lex);
}

/* ----- Edge cases ------------------------------------------------------- */

static void test_null_env_returns_null(void)
{
    FlareLexer *lex = flare_lexer_commonmark(NULL);
    ASSERT_NULL(lex);
}

static void test_empty_input(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 42;
    FlareToken *tokens = flare_lex(lex, "", 0, &count);
    ASSERT_EQ(count, 0);
    free(tokens);
    flare_lexer_free(lex);
}

static void test_plain_text(void)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    size_t count = 0;
    FlareToken *tokens = flare_lex(lex, "just text\n", 10, &count);
    ASSERT_TRUE(count > 0);
    ASSERT_EQ(tokens[0].type, HL_TEXT);
    free(tokens);
    flare_lexer_free(lex);
}

int main(void)
{
    env = lisp_init();

    /* Block structure */
    RUN_TEST(test_atx_heading);
    RUN_TEST(test_atx_heading_level2);
    RUN_TEST(test_thematic_break);
    RUN_TEST(test_setext_heading);
    RUN_TEST(test_block_quote);
    RUN_TEST(test_bullet_list);
    RUN_TEST(test_ordered_list);
    RUN_TEST(test_indented_code);

    /* Fenced code blocks */
    RUN_TEST(test_fenced_code_plain);
    RUN_TEST(test_fenced_code_lisp_suppresses_fences);
    RUN_TEST(test_fenced_code_lisp_sublexes_content);
    RUN_TEST(test_fenced_code_bloom_lisp_alias);
    RUN_TEST(test_fenced_code_bloom_alias);
    RUN_TEST(test_fenced_code_tilde);
    RUN_TEST(test_fenced_code_info_string);

    /* Inline constructs */
    RUN_TEST(test_inline_emphasis);
    RUN_TEST(test_inline_strong);
    RUN_TEST(test_inline_code_span);
    RUN_TEST(test_inline_link);
    RUN_TEST(test_inline_image);
    RUN_TEST(test_inline_escape);
    RUN_TEST(test_inline_entity);
    RUN_TEST(test_hard_break_spaces);
    RUN_TEST(test_hard_break_backslash);
    RUN_TEST(test_autolink);

    /* Edge cases */
    RUN_TEST(test_null_env_returns_null);
    RUN_TEST(test_empty_input);
    RUN_TEST(test_plain_text);

    lisp_cleanup();
    TEST_SUMMARY();
}

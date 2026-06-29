/* test_formatter.c - Formatter ANSI output tests */

#include "flare_testkit.h"
#include "lisp.h"
#include <ditty/highlight.h>
#include <string.h>

static Environment *env;

static void test_formatter_truecolor_produces_rgb(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("\"hello\"", 7, lex, style, fmt);
    ASSERT_TRUE(strstr(r.data, "38;2;") != NULL);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_256_produces_palette_index(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_256);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("\"hello\"", 7, lex, style, fmt);
    ASSERT_TRUE(strstr(r.data, "38;5;") != NULL);
    ASSERT_TRUE(strstr(r.data, "38;2;") == NULL);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_16_produces_aixterm_ansi(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_16);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("\"hello\"", 7, lex, style, fmt);
    ASSERT_TRUE(strstr(r.data, "38;") == NULL);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_8_produces_basic_ansi(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_8);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("\"hello\"", 7, lex, style, fmt);
    ASSERT_TRUE(strstr(r.data, "38;") == NULL);
    ASSERT_TRUE(strstr(r.data, "\033[90") == NULL);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_ends_with_reset(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("(+ 1 2)", 7, lex, style, fmt);
    size_t len = r.length;
    ASSERT_TRUE(len >= 4);
    ASSERT_STR_EQ(r.data + len - 4, "\033[0m");
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_bold_does_not_leak_into_next_token(void)
{
    /* Regression: bold from HL_KEYWORD_DEFINE (e.g. "defun", "set!")
     * was persisting into subsequent non-bold tokens because the SGR
     * sequence only set new attributes without first resetting old ones.
     * ANSI attributes are cumulative — \033[38;2;...m does NOT cancel
     * a prior \033[1m.  Every SGR must lead with 0 (reset). */
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("(set! x 1)", 10, lex, style, fmt);

    /* Find the SGR right after the bold "set!" token.
     * The next token should start with \033[0; (reset) and NOT contain
     * ";1;" (bold) since HL_NAME_VARIABLE and HL_PUNCTUATION are non-bold. */
    const char *set_sgr = strstr(r.data, "\033[0;1;");
    ASSERT_NOT_NULL(set_sgr); /* "set!" itself must be bold */

    /* Scan SGR sequences after the bold one: none should contain ";1;"
     * (bold parameter) unless the token actually requests bold. */
    const char *p = set_sgr + 4; /* skip past the bold SGR intro */
    while ((p = strstr(p, "\033[")) != NULL) {
        /* Find the 'm' that ends this SGR */
        const char *m = strchr(p, 'm');
        if (!m)
            break;
        /* The trailing \033[0m reset is expected — skip it */
        if (m - p == 3 && p[1] == '0') {
            p = m + 1;
            continue;
        }
        /* Check that ";1;" does not appear inside the SGR params */
        size_t sgr_len = m - p;
        char sgr_buf[128];
        ASSERT_TRUE(sgr_len < sizeof(sgr_buf));
        memcpy(sgr_buf, p, sgr_len);
        sgr_buf[sgr_len] = '\0';
        /* ";1;" = bold as a non-leading param; "0;1;" = reset+bold (ok) */
        ASSERT_TRUE(strstr(sgr_buf, ";1;") == NULL);
        p = m + 1;
    }

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

static void test_formatter_coalesces_same_style(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_monokai();
    FlareLexer *lex = flare_lexer_ditty(env);
    FlareResult r = flare_highlight("(1 2 3)", 7, lex, style, fmt);
    int sgr_count = 0;
    for (size_t i = 0; i < r.length; i++) {
        if (r.data[i] == '\033' && r.data[i + 1] == '[')
            sgr_count++;
    }
    ASSERT_TRUE(sgr_count <= 8);
    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Non-lisp fenced code blocks also have fence markers suppressed.
 * The opening/closing ``` and info strings are structural, not content. */
static void test_formatter_plain_fenced_suppresses_markers(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_dracula();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("```ruby\ncode\n```\n", 16,
                                    lex, style, fmt);

    /* Strip ANSI escapes to get plain text */
    char plain[256];
    size_t j = 0;
    for (size_t i = 0; i < r.length && j < sizeof(plain) - 1; i++) {
        if (r.data[i] == '\033') {
            while (i < r.length && r.data[i] != 'm')
                i++;
            continue;
        }
        plain[j++] = r.data[i];
    }
    plain[j] = '\0';

    /* Fence markers and info string should NOT appear in output */
    ASSERT_TRUE(strstr(plain, "```") == NULL);
    ASSERT_TRUE(strstr(plain, "ruby") == NULL);
    /* Code content should appear */
    ASSERT_TRUE(strstr(plain, "code") != NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Regression: fenced code block markers and their line-ending newlines
 * must be suppressed from rendered output.  Previously the opening fence
 * newline was emitted as a standalone HL_TEXT token, producing a spurious
 * blank line between the preceding paragraph and the code content.
 * Input:  "Text\n\n```lisp\n'()\n```\n"
 * Output should have exactly one blank line (paragraph separator)
 * between "Text" and "'()", not two. */
static void test_formatter_fenced_no_spurious_blank_line(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_dracula();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("Text\n\n```lisp\n'()\n```\n", 22,
                                    lex, style, fmt);

    /* Strip ANSI escapes to get plain text */
    char plain[256];
    size_t j = 0;
    for (size_t i = 0; i < r.length && j < sizeof(plain) - 1; i++) {
        if (r.data[i] == '\033') {
            while (i < r.length && r.data[i] != 'm')
                i++;
            continue;
        }
        plain[j++] = r.data[i];
    }
    plain[j] = '\0';

    /* Expected: "Text\n\n  '()\n" — one blank line, not two.
     * A spurious blank line would produce "Text\n\n\n  '()\n"
     * (three consecutive newlines). */
    const char *triple = strstr(plain, "\n\n\n");
    ASSERT_TRUE(triple == NULL);

    /* Verify the content is actually present */
    ASSERT_TRUE(strstr(plain, "Text") != NULL);
    ASSERT_TRUE(strstr(plain, "'()") != NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

/* Regression: blank lines within and around fenced code blocks must be
 * preserved.  The formatter should only suppress the fence markers and
 * their line-ending newlines, not legitimate blank lines:
 *   - blank line between preceding text and code block (paragraph sep)
 *   - blank line within the code block (between code lines)
 *   - blank line after the code block (paragraph sep) */
static void test_formatter_fenced_preserves_blank_lines(void)
{
    FlareFormatter *fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
    FlareStyle *style = flare_style_dracula();
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareResult r = flare_highlight("Text\n\n```lisp\nfoo\n\nbar\n```\n\nAfter\n",
                                    34, lex, style, fmt);

    /* Strip ANSI escapes to get plain text */
    char plain[512];
    size_t j = 0;
    for (size_t i = 0; i < r.length && j < sizeof(plain) - 1; i++) {
        if (r.data[i] == '\033') {
            while (i < r.length && r.data[i] != 'm')
                i++;
            continue;
        }
        plain[j++] = r.data[i];
    }
    plain[j] = '\0';

    /* Expected: "Text\n\n  foo\n  \n  bar\n\nAfter\n"
     * Fenced code content is indented by 2 spaces.
     * - blank line between Text and indented foo (paragraph separator)
     * - blank line within the code block (between foo and bar,
     *   also indented)
     * - blank line between bar and After (paragraph separator) */
    ASSERT_TRUE(strstr(plain, "Text\n\n") != NULL);
    ASSERT_TRUE(strstr(plain, "  foo\n  \n  bar") != NULL);
    ASSERT_TRUE(strstr(plain, "bar\n\nAfter") != NULL);

    flare_result_free(r);
    flare_lexer_free(lex);
    flare_style_free(style);
    flare_formatter_free(fmt);
}

int main(void)
{
    env = lisp_init();
    RUN_TEST(test_formatter_truecolor_produces_rgb);
    RUN_TEST(test_formatter_256_produces_palette_index);
    RUN_TEST(test_formatter_16_produces_aixterm_ansi);
    RUN_TEST(test_formatter_8_produces_basic_ansi);
    RUN_TEST(test_formatter_ends_with_reset);
    RUN_TEST(test_formatter_bold_does_not_leak_into_next_token);
    RUN_TEST(test_formatter_coalesces_same_style);
    RUN_TEST(test_formatter_plain_fenced_suppresses_markers);
    RUN_TEST(test_formatter_fenced_no_spurious_blank_line);
    RUN_TEST(test_formatter_fenced_preserves_blank_lines);
    lisp_cleanup();
    TEST_SUMMARY();
}

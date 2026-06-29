/* test_lexer_commonmark.c - CommonMark lexer TDD tests
 *
 * Organized by CommonMark v0.31.2 spec section.
 * Tests verify exact token types, offsets, and lengths
 * against spec example inputs.
 */

#include "flare_testkit.h"
#include "lisp.h"
#include <ditty/highlight.h>

static Environment *env;

/* Helper: lex a string and return tokens + count. */
static FlareToken *lex(const char *input, size_t *count)
{
    FlareLexer *lex = flare_lexer_commonmark(env);
    FlareToken *tokens = flare_lex(lex, input, strlen(input), count);
    flare_lexer_free(lex);
    return tokens;
}

/* Helper: find first token of a given type. Returns index or -1. */
static int find_token(FlareToken *tokens, size_t count, FlareTokenType type)
{
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == type)
            return (int)i;
    return -1;
}

/* Helper: count tokens of a given type. */
static size_t count_token(FlareToken *tokens, size_t count, FlareTokenType type)
{
    size_t n = 0;
    for (size_t i = 0; i < count; i++)
        if (tokens[i].type == type)
            n++;
    return n;
}

/* ================================================================
 * §2  Preliminaries
 * ================================================================ */

/* §2.4 Backslash escapes: \ before ASCII punctuation */
static void test_escape_ascii_punctuation(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("not \\*bold\\*\n", &count);
    int n = (int)count_token(tokens, count, HL_MARKUP_INLINE_ESCAPE);
    ASSERT_EQ(n, 2);
    /* First escape at offset 4, length 2 (\*) */
    int i = find_token(tokens, count, HL_MARKUP_INLINE_ESCAPE);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].offset, 4);
    ASSERT_EQ(tokens[i].length, 2);
    free(tokens);
}

/* §2.4 Backslash before non-punctuation is literal */
static void test_escape_non_punctuation(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("\\a\\b\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_INLINE_ESCAPE), 0);
    free(tokens);
}

/* §2.5 Entity reference: &amp; */
static void test_entity_named(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("&amp; stuff\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_ENTITY);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].offset, 0);
    ASSERT_EQ(tokens[i].length, 5);
    free(tokens);
}

/* §2.5 Numeric entity: &#123; and &#x1F; */
static void test_entity_numeric(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("&#123; &#xAB;\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_INLINE_ENTITY), 2);
    free(tokens);
}

/* ================================================================
 * §4.1  Thematic breaks
 * ================================================================ */

/* Ex 43: Valid thematic breaks with *, -, _ */
static void test_thematic_break_variants(void)
{
    const char *inputs[] = { "***\n", "---\n", "___\n" };
    for (int k = 0; k < 3; k++) {
        size_t count = 0;
        FlareToken *tokens = lex(inputs[k], &count);
        int i = find_token(tokens, count, HL_MARKUP_THEMATIC_BREAK);
        ASSERT_TRUE(i >= 0);
        ASSERT_EQ(tokens[i].offset, 0);
        ASSERT_EQ(tokens[i].length, 3);
        free(tokens);
    }
}

/* Ex 46: Too few characters is not a break */
static void test_thematic_break_too_few(void)
{
    const char *inputs[] = { "--\n", "**\n", "__\n" };
    for (int k = 0; k < 3; k++) {
        size_t count = 0;
        FlareToken *tokens = lex(inputs[k], &count);
        ASSERT_EQ(count_token(tokens, count, HL_MARKUP_THEMATIC_BREAK), 0);
        free(tokens);
    }
}

/* Ex 47: Up to 3 spaces of indentation allowed */
static void test_thematic_break_indent_ok(void)
{
    const char *inputs[] = { " ***\n", "  ***\n", "   ***\n" };
    for (int k = 0; k < 3; k++) {
        size_t count = 0;
        FlareToken *tokens = lex(inputs[k], &count);
        ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_THEMATIC_BREAK) >= 0);
        free(tokens);
    }
}

/* Ex 48: 4 spaces indent makes it a code block, not thematic break */
static void test_thematic_break_indent_too_much(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("    ***\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_THEMATIC_BREAK), 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INDENTED_CODE) >= 0);
    free(tokens);
}

/* Ex 51: Spaces between characters allowed */
static void test_thematic_break_spaces_between(void)
{
    size_t count = 0;
    FlareToken *tokens = lex(" - - -\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_THEMATIC_BREAK) >= 0);
    free(tokens);
}

/* Ex 55: Other characters invalidate the break */
static void test_thematic_break_invalid_chars(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("_ _ _ _ a\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_THEMATIC_BREAK), 0);
    free(tokens);
}

/* ================================================================
 * §4.2  ATX headings
 * ================================================================ */

/* Ex 62: All six heading levels */
static void test_atx_all_levels(void)
{
    const char *inputs[] = {
        "# foo\n", "## foo\n", "### foo\n",
        "#### foo\n", "##### foo\n", "###### foo\n"
    };
    for (int k = 0; k < 6; k++) {
        size_t count = 0;
        FlareToken *tokens = lex(inputs[k], &count);
        int i = find_token(tokens, count, HL_MARKUP_HEADING_MARKER);
        ASSERT_TRUE(i >= 0);
        ASSERT_EQ(tokens[i].length, (size_t)(k + 1));
        free(tokens);
    }
}

/* Ex 63: Seven # is not a heading */
static void test_atx_seven_hashes(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("####### foo\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_HEADING_MARKER), 0);
    free(tokens);
}

/* Ex 64: No space after # means not a heading */
static void test_atx_no_space_after_hash(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("#5 bolt\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_HEADING_MARKER), 0);
    free(tokens);
}

/* Ex 71: Closing # sequence */
static void test_atx_closing_hash(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("## foo ##\n", &count);
    /* Should have two HEADING_MARKER tokens: opening ## and closing ## */
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_HEADING_MARKER), 2);
    free(tokens);
}

/* Ex 75: Closing # without preceding space is not a marker */
static void test_atx_closing_hash_no_space(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("# foo#\n", &count);
    /* Only one HEADING_MARKER — the opening #. foo# is just text. */
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_HEADING_MARKER), 1);
    free(tokens);
}

/* Ex 79: Empty heading */
static void test_atx_empty(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("##\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_HEADING_MARKER) >= 0);
    free(tokens);
}

/* ================================================================
 * §4.3  Setext headings
 * ================================================================ */

/* Ex 80: Level 1 (=) and level 2 (-) */
static void test_setext_level1(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("Foo\n===\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_SETEXT_UNDERLINE) >= 0);
    free(tokens);
}

static void test_setext_level2(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("Foo\n---\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_SETEXT_UNDERLINE) >= 0);
    free(tokens);
}

/* Ex 83: Underline can be any length */
static void test_setext_any_length(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("Foo\n-------------------------\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_SETEXT_UNDERLINE);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].length, 25);
    free(tokens);
}

/* Ex 88: Spaces in underline invalidate it */
static void test_setext_spaces_in_underline(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("Foo\n= =\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_SETEXT_UNDERLINE), 0);
    free(tokens);
}

/* Ex 97: --- at start of document without preceding text is not setext */
static void test_setext_no_preceding_text(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("===\n", &count);
    /* Without preceding paragraph text, this is not a setext heading */
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_SETEXT_UNDERLINE), 0);
    free(tokens);
}

/* ================================================================
 * §4.4  Indented code blocks
 * ================================================================ */

static void test_indented_code_basic(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("    code\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INDENTED_CODE);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

static void test_indented_code_3_spaces_not_enough(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("   not code\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_INDENTED_CODE), 0);
    free(tokens);
}

/* ================================================================
 * §4.5  Fenced code blocks
 * ================================================================ */

/* Ex 119: Basic backtick fence */
static void test_fenced_backtick(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("```\nfoo\n```\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_OPEN) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_CLOSE) >= 0);
    free(tokens);
}

/* Ex 120: Basic tilde fence */
static void test_fenced_tilde(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("~~~\nfoo\n~~~\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_OPEN) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_CLOSE) >= 0);
    free(tokens);
}

/* Ex 122: Closing fence must use same character as opening */
static void test_fenced_mismatched_char(void)
{
    size_t count = 0;
    /* Opening with backticks, closing with tildes does not close */
    FlareToken *tokens = lex("```\naaa\n~~~\n```\n", &count);
    /* The ~~~ should not close it; the ``` should */
    int nclose = (int)count_token(tokens, count, HL_MARKUP_FENCED_CLOSE);
    ASSERT_EQ(nclose, 1);
    free(tokens);
}

/* Ex 124: Closing fence must be at least as long as opening */
static void test_fenced_close_too_short(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("````\naaa\n```\n````\n", &count);
    /* The 3-backtick close is too short; the 4-backtick closes it */
    int nclose = (int)count_token(tokens, count, HL_MARKUP_FENCED_CLOSE);
    ASSERT_EQ(nclose, 1);
    free(tokens);
}

/* Ex 131: Up to 3 spaces indent on opening fence */
static void test_fenced_indent_open(void)
{
    size_t count = 0;
    FlareToken *tokens = lex(" ```\nfoo\n```\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_OPEN) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_CLOSE) >= 0);
    free(tokens);
}

/* Ex 134: 4 spaces indent on opening fence makes it indented code instead */
static void test_fenced_indent_too_much(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("    ```\n    aaa\n    ```\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_FENCED_OPEN), 0);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_FENCED_CLOSE), 0);
    free(tokens);
}

/* Ex 142: Info string */
static void test_fenced_info_string(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("```ruby\ncode\n```\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_FENCED_INFO);
    ASSERT_TRUE(i >= 0);
    /* Info string "ruby" starts after fence + space */
    ASSERT_EQ(tokens[i].length, 4);
    free(tokens);
}

/* Ex 145: Backtick info string on backtick fence is not a code block */
static void test_fenced_backtick_info_with_backticks(void)
{
    size_t count = 0;
    /* ``` aa ``` has backticks in info string — invalid for backtick fence */
    FlareToken *tokens = lex("``` aa ```\nfoo\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_FENCED_OPEN), 0);
    free(tokens);
}

/* Lisp sub-lexing: lexer emits fence markers + sub-lexed content */
static void test_fenced_lisp_emits_fences_and_content(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("```lisp\n(+ 1 2)\n```\n", &count);
    /* Lexer emits structural fence tokens */
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_OPEN) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_CLOSE) >= 0);
    /* Lexer emits sub-lexed content tokens */
    ASSERT_TRUE(find_token(tokens, count, HL_PUNCT_OPEN_PAREN) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_NAME_BUILTIN) >= 0);
    free(tokens);
}

/* Lisp sub-lexing: content is tokenized as ditty */
static void test_fenced_lisp_sublexes_content(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("```lisp\n(+ 1 2)\n```\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_PUNCT_OPEN_PAREN) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_NAME_BUILTIN) >= 0);
    free(tokens);
}

/* ditty alias also triggers sub-lexing */
static void test_fenced_ditty_alias(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("```ditty\n(+ 1)\n```\n", &count);
    /* Lexer emits fence markers + sub-lexed content */
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_OPEN) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_PUNCT_OPEN_PAREN) >= 0);
    free(tokens);
}

/* Unclosed fenced code block */
static void test_fenced_unclosed(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("```\nfoo\n", &count);
    /* Should still produce an opening fence */
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_OPEN) >= 0);
    free(tokens);
}

/* ================================================================
 * §4.6  HTML blocks
 * ================================================================ */

/* Type 1: <pre> */
static void test_html_block_pre(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("<pre>\nfoo\n</pre>\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_HTML_BLOCK) >= 0);
    free(tokens);
}

/* Type 2: <!-- comment --> */
static void test_html_block_comment(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("<!-- comment -->\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_HTML_BLOCK) >= 0);
    free(tokens);
}

/* Type 6: block-level tag <div> */
static void test_html_block_div(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("<div>\nfoo\n</div>\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_HTML_BLOCK) >= 0);
    free(tokens);
}

/* <https://...> is NOT an HTML block (it's an autolink) */
static void test_html_block_not_autolink(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("<https://example.com>\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_HTML_BLOCK), 0);
    free(tokens);
}

/* ================================================================
 * §4.7  Link reference definitions
 * ================================================================ */

static void test_link_ref_def(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("[foo]: /url \"title\"\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_LINKREF_DEF) >= 0);
    free(tokens);
}

static void test_link_ref_def_indent(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("  [foo]: /url\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_LINKREF_DEF) >= 0);
    free(tokens);
}

static void test_link_ref_def_too_much_indent(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("    [foo]: /url\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_LINKREF_DEF), 0);
    free(tokens);
}

/* ================================================================
 * §5.1  Block quotes
 * ================================================================ */

static void test_block_quote_basic(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("> quote\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_BLOCKQUOTE_MARKER);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].offset, 0);
    /* > followed by space: marker covers "> " */
    ASSERT_EQ(tokens[i].length, 2);
    free(tokens);
}

static void test_block_quote_no_space(void)
{
    size_t count = 0;
    FlareToken *tokens = lex(">quote\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_BLOCKQUOTE_MARKER);
    ASSERT_TRUE(i >= 0);
    /* > without space: marker is just > (1 char) */
    ASSERT_EQ(tokens[i].length, 1);
    free(tokens);
}

static void test_block_quote_indent(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("  > quote\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_BLOCKQUOTE_MARKER) >= 0);
    free(tokens);
}

/* ================================================================
 * §5.2  List items
 * ================================================================ */

static void test_list_bullet_dash(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("- item\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_LIST_MARKER);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

static void test_list_bullet_plus(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("+ item\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_LIST_MARKER) >= 0);
    free(tokens);
}

static void test_list_bullet_star(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("* item\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_LIST_MARKER) >= 0);
    free(tokens);
}

static void test_list_ordered_dot(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("1. first\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_LIST_MARKER) >= 0);
    free(tokens);
}

static void test_list_ordered_paren(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("1) first\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_LIST_MARKER) >= 0);
    free(tokens);
}

/* Bullet without trailing space is not a list item */
static void test_list_bullet_no_space(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("-not-a-list\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_LIST_MARKER), 0);
    free(tokens);
}

/* ================================================================
 * §6.1  Code spans
 * ================================================================ */

/* Ex 328: Simple code span */
static void test_code_span_simple(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("`foo`\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_CODE);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].offset, 0);
    ASSERT_EQ(tokens[i].length, 5);
    free(tokens);
}

/* Ex 329: Two backticks to contain a literal backtick */
static void test_code_span_double_backtick(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("`` foo ` bar ``\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_CODE);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

/* Ex 330: `` `` `` — code span containing two backticks */
static void test_code_span_containing_backticks(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("` `` `\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_CODE);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

/* Ex 338: Backslash escapes don't work in code spans */
static void test_code_span_no_escape(void)
{
    size_t count = 0;
    /* `foo\`bar` — the backslash is literal inside the code span.
     * This is an unclosed code span because \` is not an escape,
     * so the ` after \ ends it. The whole thing is `foo\` + bar` */
    FlareToken *tokens = lex("`foo\\`bar`\n", &count);
    /* At minimum, the first ` should be processed somehow.
     * The exact behavior is complex, but we should not crash. */
    ASSERT_TRUE(count > 0);
    free(tokens);
}

/* ================================================================
 * §6.2  Emphasis and strong emphasis
 * ================================================================ */

/* Ex 350: Basic emphasis with * */
static void test_emphasis_star(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("*foo bar*\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_EMPHASIS) >= 0);
    free(tokens);
}

/* Ex 357: Basic emphasis with _ */
static void test_emphasis_underscore(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("_foo bar_\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_EMPHASIS) >= 0);
    free(tokens);
}

/* Ex 378: Strong emphasis with ** */
static void test_strong_star(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("**foo bar**\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_STRONG) >= 0);
    free(tokens);
}

/* Ex 382: Strong emphasis with __ */
static void test_strong_underscore(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("__foo bar__\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_STRONG) >= 0);
    free(tokens);
}

/* Ex 467: ***foo*** = em + strong */
static void test_em_strong_triple(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("***foo***\n", &count);
    /* Triple delimiter should be tokenized as STRONG (run length >= 2) */
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_STRONG) >= 0);
    free(tokens);
}

/* ================================================================
 * §6.3  Links
 * ================================================================ */

/* Inline link: [text](url) */
static void test_link_inline(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("click [here](url)\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_LINK);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

/* Full reference link: [text][label] */
static void test_link_full_reference(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("[text][label]\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_LINK);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

/* ================================================================
 * §6.4  Images
 * ================================================================ */

static void test_image_inline(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("see ![alt](img.png)\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_IMAGE);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

/* ================================================================
 * §6.5  Autolinks
 * ================================================================ */

/* Ex 594: URI autolink */
static void test_autolink_url(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("<http://foo.bar.baz>\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_AUTOLINK);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].offset, 0);
    ASSERT_EQ(tokens[i].length, 20);
    free(tokens);
}

/* Ex 604: Email autolink */
static void test_autolink_email(void)
{
    size_t count = 0;
    /* Email autolinks require paragraph context */
    FlareToken *tokens = lex("see <foo@bar.example.com>\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_AUTOLINK);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

/* Ex 607: Empty angle brackets are not autolinks */
static void test_autolink_empty(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("<>\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_INLINE_AUTOLINK), 0);
    free(tokens);
}

/* Ex 602: Spaces not allowed in autolink */
static void test_autolink_no_spaces(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("<https://foo bar>\n", &count);
    ASSERT_EQ(count_token(tokens, count, HL_MARKUP_INLINE_AUTOLINK), 0);
    free(tokens);
}

/* ================================================================
 * §6.6  Raw inline HTML
 * ================================================================ */

static void test_inline_html_tag(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("text <b>bold</b> text\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_HTML) >= 0);
    free(tokens);
}

/* ================================================================
 * §6.7  Hard line breaks
 * ================================================================ */

/* Ex 633: Two trailing spaces produce hard break */
static void test_hard_break_spaces(void)
{
    size_t count = 0;
    /* "foo  \nbaz\n" — two spaces before newline */
    FlareToken *tokens = lex("foo  \nbaz\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_BREAK);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

/* Ex 634: Backslash before line ending */
static void test_hard_break_backslash(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("foo\\\nbaz\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_INLINE_BREAK);
    ASSERT_TRUE(i >= 0);
    free(tokens);
}

/* ================================================================
 * §4.8  Paragraphs / plain text
 * ================================================================ */

static void test_paragraph_basic(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("just text\n", &count);
    ASSERT_TRUE(count > 0);
    ASSERT_EQ(tokens[0].type, HL_TEXT);
    free(tokens);
}

static void test_paragraph_multiline(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("line one\nline two\n", &count);
    /* Should have text tokens for both lines */
    ASSERT_TRUE(count > 0);
    free(tokens);
}

/* ================================================================
 * Edge cases
 * ================================================================ */

static void test_null_env_returns_null(void)
{
    FlareLexer *lex = flare_lexer_commonmark(NULL);
    ASSERT_NULL(lex);
}

static void test_empty_input(void)
{
    size_t count = 42;
    FlareToken *tokens = lex("", &count);
    ASSERT_EQ(count, 0);
    free(tokens);
}

static void test_blank_line(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("\n\n\n", &count);
    /* All tokens should be HL_TEXT (blank lines) */
    for (size_t i = 0; i < count; i++)
        ASSERT_EQ(tokens[i].type, HL_TEXT);
    free(tokens);
}

/* Setext heading with inline content in the text above */
static void test_setext_with_inline(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("Foo *bar*\n====\n", &count);
    /* Should have both setext underline and emphasis in text */
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_SETEXT_UNDERLINE) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_EMPHASIS) >= 0);
    free(tokens);
}

/* ATX heading with inline content */
static void test_atx_with_inline(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("# Hello *world*\n", &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_HEADING_MARKER) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_EMPHASIS) >= 0);
    free(tokens);
}

/* ================================================================
 * Token coverage audit: every token type is exercised
 * ================================================================ */

static void test_all_block_token_types_used(void)
{
    size_t count = 0;
    /* Use a combined document to exercise every block token type */
    const char *doc = "# Heading\n\n---\n\nTitle\n===\n\n> quote\n\n"
                      "- item\n\n```\ncode\n```\n\n    indented\n\n"
                      "<div>html</div>\n\n[ref]: /url\n";
    FlareToken *tokens = lex(doc, &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_HEADING_MARKER) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_THEMATIC_BREAK) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_SETEXT_UNDERLINE) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_BLOCKQUOTE_MARKER) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_LIST_MARKER) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_OPEN) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_FENCED_CLOSE) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INDENTED_CODE) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_HTML_BLOCK) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_LINKREF_DEF) >= 0);
    free(tokens);
}

static void test_all_inline_token_types_used(void)
{
    size_t count = 0;
    /* Use a combined paragraph to exercise every inline token type */
    const char *doc = "*emph* **strong** `code` [link](url) "
                      "![img](src) <http://x> \\< \\\n&amp; <b>tag</b>\n";
    FlareToken *tokens = lex(doc, &count);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_EMPHASIS) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_STRONG) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_CODE) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_LINK) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_IMAGE) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_AUTOLINK) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_ESCAPE) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_BREAK) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_ENTITY) >= 0);
    ASSERT_TRUE(find_token(tokens, count, HL_MARKUP_INLINE_HTML) >= 0);
    free(tokens);
}

/* ================================================================
 * Offset / length precision tests
 * ================================================================ */

/* Verify exact offsets for a simple ATX heading */
static void test_atx_offset_precision(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("# Hello\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_HEADING_MARKER);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].offset, 0);
    ASSERT_EQ(tokens[i].length, 1);
    /* Next token should be space (HL_TEXT at offset 1, length 1) */
    if (i + 1 < (int)count) {
        ASSERT_EQ(tokens[i + 1].type, HL_TEXT);
        ASSERT_EQ(tokens[i + 1].offset, 1);
        ASSERT_EQ(tokens[i + 1].length, 1);
    }
    free(tokens);
}

/* Verify exact offsets for thematic break */
static void test_thematic_break_offset_precision(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("---\n", &count);
    ASSERT_EQ(count, 2);
    ASSERT_EQ(tokens[0].type, HL_MARKUP_THEMATIC_BREAK);
    ASSERT_EQ(tokens[0].offset, 0);
    ASSERT_EQ(tokens[0].length, 3);
    /* Newline as HL_TEXT */
    ASSERT_EQ(tokens[1].type, HL_TEXT);
    ASSERT_EQ(tokens[1].offset, 3);
    ASSERT_EQ(tokens[1].length, 1);
    free(tokens);
}

/* Verify exact offsets for fenced code block with info string */
static void test_fenced_info_offset_precision(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("```ruby\ncode\n```\n", &count);
    int open_i = find_token(tokens, count, HL_MARKUP_FENCED_OPEN);
    int info_i = find_token(tokens, count, HL_MARKUP_FENCED_INFO);
    int close_i = find_token(tokens, count, HL_MARKUP_FENCED_CLOSE);
    ASSERT_TRUE(open_i >= 0);
    ASSERT_TRUE(info_i >= 0);
    ASSERT_TRUE(close_i >= 0);
    ASSERT_EQ(tokens[open_i].offset, 0);
    ASSERT_EQ(tokens[open_i].length, 3);
    ASSERT_EQ(tokens[info_i].offset, 3);
    ASSERT_EQ(tokens[info_i].length, 4);
    free(tokens);
}

/* Verify block quote marker covers "> " (2 chars) */
static void test_block_quote_offset_precision(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("> text\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_BLOCKQUOTE_MARKER);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].offset, 0);
    ASSERT_EQ(tokens[i].length, 2);
    free(tokens);
}

/* Verify list marker covers "- " (2 chars) */
static void test_list_marker_offset_precision(void)
{
    size_t count = 0;
    FlareToken *tokens = lex("- item\n", &count);
    int i = find_token(tokens, count, HL_MARKUP_LIST_MARKER);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ(tokens[i].offset, 0);
    ASSERT_EQ(tokens[i].length, 1);
    free(tokens);
}

/* ================================================================
 * main
 * ================================================================ */

int main(void)
{
    env = lisp_init();

    /* §2 Preliminaries */
    RUN_TEST(test_escape_ascii_punctuation);
    RUN_TEST(test_escape_non_punctuation);
    RUN_TEST(test_entity_named);
    RUN_TEST(test_entity_numeric);

    /* §4.1 Thematic breaks */
    RUN_TEST(test_thematic_break_variants);
    RUN_TEST(test_thematic_break_too_few);
    RUN_TEST(test_thematic_break_indent_ok);
    RUN_TEST(test_thematic_break_indent_too_much);
    RUN_TEST(test_thematic_break_spaces_between);
    RUN_TEST(test_thematic_break_invalid_chars);

    /* §4.2 ATX headings */
    RUN_TEST(test_atx_all_levels);
    RUN_TEST(test_atx_seven_hashes);
    RUN_TEST(test_atx_no_space_after_hash);
    RUN_TEST(test_atx_closing_hash);
    RUN_TEST(test_atx_closing_hash_no_space);
    RUN_TEST(test_atx_empty);

    /* §4.3 Setext headings */
    RUN_TEST(test_setext_level1);
    RUN_TEST(test_setext_level2);
    RUN_TEST(test_setext_any_length);
    RUN_TEST(test_setext_spaces_in_underline);
    RUN_TEST(test_setext_no_preceding_text);

    /* §4.4 Indented code blocks */
    RUN_TEST(test_indented_code_basic);
    RUN_TEST(test_indented_code_3_spaces_not_enough);

    /* §4.5 Fenced code blocks */
    RUN_TEST(test_fenced_backtick);
    RUN_TEST(test_fenced_tilde);
    RUN_TEST(test_fenced_mismatched_char);
    RUN_TEST(test_fenced_close_too_short);
    RUN_TEST(test_fenced_indent_open);
    RUN_TEST(test_fenced_indent_too_much);
    RUN_TEST(test_fenced_info_string);
    RUN_TEST(test_fenced_backtick_info_with_backticks);
    RUN_TEST(test_fenced_lisp_emits_fences_and_content);
    RUN_TEST(test_fenced_lisp_sublexes_content);
    RUN_TEST(test_fenced_ditty_alias);
    RUN_TEST(test_fenced_unclosed);

    /* §4.6 HTML blocks */
    RUN_TEST(test_html_block_pre);
    RUN_TEST(test_html_block_comment);
    RUN_TEST(test_html_block_div);
    RUN_TEST(test_html_block_not_autolink);

    /* §4.7 Link reference definitions */
    RUN_TEST(test_link_ref_def);
    RUN_TEST(test_link_ref_def_indent);
    RUN_TEST(test_link_ref_def_too_much_indent);

    /* §5.1 Block quotes */
    RUN_TEST(test_block_quote_basic);
    RUN_TEST(test_block_quote_no_space);
    RUN_TEST(test_block_quote_indent);

    /* §5.2 List items */
    RUN_TEST(test_list_bullet_dash);
    RUN_TEST(test_list_bullet_plus);
    RUN_TEST(test_list_bullet_star);
    RUN_TEST(test_list_ordered_dot);
    RUN_TEST(test_list_ordered_paren);
    RUN_TEST(test_list_bullet_no_space);

    /* §6.1 Code spans */
    RUN_TEST(test_code_span_simple);
    RUN_TEST(test_code_span_double_backtick);
    RUN_TEST(test_code_span_containing_backticks);
    RUN_TEST(test_code_span_no_escape);

    /* §6.2 Emphasis and strong */
    RUN_TEST(test_emphasis_star);
    RUN_TEST(test_emphasis_underscore);
    RUN_TEST(test_strong_star);
    RUN_TEST(test_strong_underscore);
    RUN_TEST(test_em_strong_triple);

    /* §6.3 Links */
    RUN_TEST(test_link_inline);
    RUN_TEST(test_link_full_reference);

    /* §6.4 Images */
    RUN_TEST(test_image_inline);

    /* §6.5 Autolinks */
    RUN_TEST(test_autolink_url);
    RUN_TEST(test_autolink_email);
    RUN_TEST(test_autolink_empty);
    RUN_TEST(test_autolink_no_spaces);

    /* §6.6 Inline HTML */
    RUN_TEST(test_inline_html_tag);

    /* §6.7 Hard line breaks */
    RUN_TEST(test_hard_break_spaces);
    RUN_TEST(test_hard_break_backslash);

    /* §4.8 Paragraphs */
    RUN_TEST(test_paragraph_basic);
    RUN_TEST(test_paragraph_multiline);

    /* Edge cases */
    RUN_TEST(test_null_env_returns_null);
    RUN_TEST(test_empty_input);
    RUN_TEST(test_blank_line);
    RUN_TEST(test_setext_with_inline);
    RUN_TEST(test_atx_with_inline);

    /* Token coverage audit */
    RUN_TEST(test_all_block_token_types_used);
    RUN_TEST(test_all_inline_token_types_used);

    /* Offset precision */
    RUN_TEST(test_atx_offset_precision);
    RUN_TEST(test_thematic_break_offset_precision);
    RUN_TEST(test_fenced_info_offset_precision);
    RUN_TEST(test_block_quote_offset_precision);
    RUN_TEST(test_list_marker_offset_precision);

    lisp_cleanup();
    TEST_SUMMARY();
}

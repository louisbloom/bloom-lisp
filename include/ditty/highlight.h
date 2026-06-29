/* highlight.h - Public API for ditty flare syntax highlighting
 *
 * A Chroma-inspired, pure-C syntax highlighting library producing
 * ANSI terminal output at 16-color, 256-color, and truecolor depths.
 */

#ifndef BLOOM_FLARE_HIGHLIGHT_H
#define BLOOM_FLARE_HIGHLIGHT_H

#include "lisp.h"
#include <stddef.h>
#include <stdint.h>

/* ----- Token type constants --------------------------------------------- */

typedef int FlareTokenType;

#define HL_TEXT                      0
#define HL_KEYWORD                   1000
#define HL_KEYWORD_SPECIAL_FORM      1010
#define HL_KEYWORD_DEFINE            1011
#define HL_KEYWORD_CONTROL           1012
#define HL_KEYWORD_MACRO_DEF         1013
#define HL_NAME                      2000
#define HL_NAME_FUNCTION             2010
#define HL_NAME_BUILTIN              2020
#define HL_NAME_VARIABLE             2030
#define HL_NAME_CONSTANT             2040
#define HL_NAME_KEYWORD_ARG          2050
#define HL_NAME_PACKAGE              2060
#define HL_LITERAL                   3000
#define HL_LITERAL_STRING            3010
#define HL_LITERAL_STRING_ESCAPE     3011
#define HL_LITERAL_NUMBER            3020
#define HL_LITERAL_CHARACTER         3030
#define HL_LITERAL_BOOLEAN           3040
#define HL_LITERAL_NIL               3050
#define HL_OPERATOR                  4000
#define HL_OPERATOR_QUOTE            4010
#define HL_OPERATOR_BACKQUOTE        4020
#define HL_OPERATOR_UNQUOTE          4030
#define HL_OPERATOR_UNQUOTE_SPLICING 4040
#define HL_PUNCTUATION               5000
#define HL_PUNCT_OPEN_PAREN          5010
#define HL_PUNCT_CLOSE_PAREN         5020
#define HL_PUNCT_DOT                 5030
#define HL_PUNCT_HASH                5040
#define HL_COMMENT                   6000
#define HL_COMMENT_LINE              6010
#define HL_ERROR                     7000
#define HL_ERROR_UNCLOSED_STRING     7010
#define HL_ERROR_UNCLOSED_PAREN      7020

/* ----- CommonMark / Markdown token types -------------------------------- */

/* Block structure (category 8000) */
#define HL_MARKUP                   8000
#define HL_MARKUP_HEADING           8010
#define HL_MARKUP_HEADING_MARKER    8011
#define HL_MARKUP_SETEXT_UNDERLINE  8020
#define HL_MARKUP_FENCED_OPEN       8030
#define HL_MARKUP_FENCED_INFO       8031
#define HL_MARKUP_FENCED_CLOSE      8032
#define HL_MARKUP_INDENTED_CODE     8040
#define HL_MARKUP_BLOCKQUOTE_MARKER 8050
#define HL_MARKUP_LIST_MARKER       8060
#define HL_MARKUP_THEMATIC_BREAK    8070
#define HL_MARKUP_HTML_BLOCK        8080
#define HL_MARKUP_LINKREF_DEF       8090

/* Inline structure (category 9000) */
#define HL_MARKUP_INLINE          9000
#define HL_MARKUP_INLINE_EMPHASIS 9010
#define HL_MARKUP_INLINE_STRONG   9020
#define HL_MARKUP_INLINE_CODE     9030
#define HL_MARKUP_INLINE_LINK     9040
#define HL_MARKUP_INLINE_IMAGE    9050
#define HL_MARKUP_INLINE_AUTOLINK 9060
#define HL_MARKUP_INLINE_BREAK    9070
#define HL_MARKUP_INLINE_ESCAPE   9080
#define HL_MARKUP_INLINE_ENTITY   9090
#define HL_MARKUP_INLINE_HTML     9100

/* ----- Token type helpers ----------------------------------------------- */

/* Return the category (range base) for a token type:
 *   0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000 */
int flare_token_category(FlareTokenType type);

/* Return the subcategory for a token type:
 *   e.g. HL_KEYWORD_SPECIAL_FORM → 1010 */
int flare_token_subcategory(FlareTokenType type);

/* ----- Core types ------------------------------------------------------- */

/* Token produced by the lexer */
typedef struct
{
    FlareTokenType type;
    size_t offset; /* byte offset into source */
    size_t length; /* byte length of the token text */
} FlareToken;

/* Styling for a token type */
typedef struct
{
    uint8_t fg_r, fg_g, fg_b;
    uint8_t bg_r, bg_g, bg_b;
    int bold, italic, underline, faint, strikethrough;
} FlareStyleEntry;

/* A complete style (collection of type → entry mappings) */
typedef struct FlareStyle FlareStyle;

/* A lexer (language-specific tokenizer) */
typedef struct FlareLexer FlareLexer;

/* A formatter (produces output from token stream + style) */
typedef struct FlareFormatter FlareFormatter;

/* Result of highlighting: ANSI-escaped string */
typedef struct
{
    char *data;    /* NUL-terminated ANSI string */
    size_t length; /* byte length (not counting NUL) */
} FlareResult;

/* ----- Lexer API -------------------------------------------------------- */

/* Create a Bloom Lisp lexer. Requires a non-NULL Environment* (obtained
 * from lisp_init()) for semantic classification of symbols as
 * special forms, builtins, macros, or variables. Passing NULL
 * returns NULL. */
FlareLexer *flare_lexer_ditty(Environment *env);

/* Create a CommonMark/Markdown lexer. Requires a non-NULL Environment*
 * for sub-lexing fenced code blocks with the ditty lexer.
 * Passing NULL returns NULL. */
FlareLexer *flare_lexer_commonmark(Environment *env);

/* Generic lexer by name (future: "c", "python", etc.) */
FlareLexer *flare_lexer_get(const char *name);

/* Tokenize input. Returns a malloc'd array of tokens;
 * *out_count receives the number of tokens (0 for empty input).
 * Caller frees the array with free(). */
FlareToken *flare_lex(const FlareLexer *lexer, const char *input,
                      size_t input_len, size_t *out_count);

void flare_lexer_free(FlareLexer *lexer);

/* ----- Style API ------------------------------------------------------- */

/* Built-in styles */
FlareStyle *flare_style_monokai(void);
FlareStyle *flare_style_dracula(void);
FlareStyle *flare_style_github_dark(void);
FlareStyle *flare_style_github_light(void);

/* Look up style entry for a token type (walks hierarchy) */
FlareStyleEntry flare_style_get(const FlareStyle *style, FlareTokenType type);

/* Custom style building */
FlareStyle *flare_style_new(void);
void flare_style_set(FlareStyle *style, FlareTokenType type,
                     const FlareStyleEntry *entry);

void flare_style_free(FlareStyle *style);

/* ----- Formatter API --------------------------------------------------- */

/* Color depth for formatters */
typedef enum
{
    BFLARE_COLOR_8,        /* ANSI 8-color (SGR 30-37, bold for bright) */
    BFLARE_COLOR_16,       /* ANSI 16-color (SGR 30-37, 90-97) */
    BFLARE_COLOR_256,      /* xterm-256 (SGR 38;5;N) */
    BFLARE_COLOR_TRUECOLOR /* 24-bit RGB (SGR 38;2;R;G;B) */
} FlareColorDepth;

/* Create a terminal formatter at the given color depth */
FlareFormatter *flare_formatter_terminal(FlareColorDepth depth);

void flare_formatter_free(FlareFormatter *formatter);

/* ----- Color conversion API -------------------------------------------- */

/* Convert 24-bit RGB to closest 256-color palette index (0-255) */
int flare_color_rgb_to_256(int r, int g, int b);

/* Convert 24-bit RGB to closest 8-color ANSI index (0-7) */
int flare_color_rgb_to_8(int r, int g, int b);

/* Convert 24-bit RGB to closest 16-color ANSI index (0-15) via nearest-match */
int flare_color_rgb_to_16(int r, int g, int b);

/* Reverse mapping: 256-color palette index to RGB */
void flare_color_256_to_rgb(int idx, uint8_t *r, uint8_t *g, uint8_t *b);

/* ----- One-shot highlight API ------------------------------------------ */

/* Highlight source text in one call.
 *
 * lexer, style, formatter are all optional (NULL = defaults:
 * ditty lexer, monokai style, truecolor terminal formatter).
 *
 * Returns a FlareResult with a malloc'd ANSI string.
 * Caller frees with flare_result_free(). */
FlareResult flare_highlight(const char *input, size_t input_len,
                            const FlareLexer *lexer,
                            const FlareStyle *style,
                            const FlareFormatter *formatter);

void flare_result_free(FlareResult result);

#endif /* BLOOM_FLARE_HIGHLIGHT_H */

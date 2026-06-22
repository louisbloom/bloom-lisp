/* highlight.h - Public API for bloom-flare syntax highlighting
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

typedef int BflareTokenType;

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

/* ----- Token type helpers ----------------------------------------------- */

/* Return the category (range base) for a token type:
 *   0, 1000, 2000, 3000, 4000, 5000, 6000, 7000 */
int bflare_token_category(BflareTokenType type);

/* Return the subcategory for a token type:
 *   e.g. HL_KEYWORD_SPECIAL_FORM → 1010 */
int bflare_token_subcategory(BflareTokenType type);

/* ----- Core types ------------------------------------------------------- */

/* Token produced by the lexer */
typedef struct
{
    BflareTokenType type;
    size_t offset; /* byte offset into source */
    size_t length; /* byte length of the token text */
} BflareToken;

/* Styling for a token type */
typedef struct
{
    uint8_t fg_r, fg_g, fg_b;
    uint8_t bg_r, bg_g, bg_b;
    int bold, italic, underline, faint, strikethrough;
} BflareStyleEntry;

/* A complete style (collection of type → entry mappings) */
typedef struct BflareStyle BflareStyle;

/* A lexer (language-specific tokenizer) */
typedef struct BflareLexer BflareLexer;

/* A formatter (produces output from token stream + style) */
typedef struct BflareFormatter BflareFormatter;

/* Result of highlighting: ANSI-escaped string */
typedef struct
{
    char *data;    /* NUL-terminated ANSI string */
    size_t length; /* byte length (not counting NUL) */
} BflareResult;

/* ----- Lexer API -------------------------------------------------------- */

/* Create a Bloom Lisp lexer. Requires a non-NULL Environment* (obtained
 * from lisp_init()) for semantic classification of symbols as
 * special forms, builtins, macros, or variables. Passing NULL
 * returns NULL. */
BflareLexer *bflare_lexer_bloom_lisp(Environment *env);

/* Generic lexer by name (future: "c", "python", etc.) */
BflareLexer *bflare_lexer_get(const char *name);

/* Tokenize input. Returns a malloc'd array of tokens;
 * *out_count receives the number of tokens (0 for empty input).
 * Caller frees the array with free(). */
BflareToken *bflare_lex(const BflareLexer *lexer, const char *input,
                        size_t input_len, size_t *out_count);

void bflare_lexer_free(BflareLexer *lexer);

/* ----- Style API ------------------------------------------------------- */

/* Built-in styles */
BflareStyle *bflare_style_monokai(void);
BflareStyle *bflare_style_dracula(void);
BflareStyle *bflare_style_github_dark(void);
BflareStyle *bflare_style_github_light(void);

/* Look up style entry for a token type (walks hierarchy) */
BflareStyleEntry bflare_style_get(const BflareStyle *style, BflareTokenType type);

/* Custom style building */
BflareStyle *bflare_style_new(void);
void bflare_style_set(BflareStyle *style, BflareTokenType type,
                      const BflareStyleEntry *entry);

void bflare_style_free(BflareStyle *style);

/* ----- Formatter API --------------------------------------------------- */

/* Color depth for formatters */
typedef enum
{
    BFLARE_COLOR_8,        /* ANSI 8-color (SGR 30-37, bold for bright) */
    BFLARE_COLOR_16,       /* ANSI 16-color (SGR 30-37, 90-97) */
    BFLARE_COLOR_256,      /* xterm-256 (SGR 38;5;N) */
    BFLARE_COLOR_TRUECOLOR /* 24-bit RGB (SGR 38;2;R;G;B) */
} BflareColorDepth;

/* Create a terminal formatter at the given color depth */
BflareFormatter *bflare_formatter_terminal(BflareColorDepth depth);

void bflare_formatter_free(BflareFormatter *formatter);

/* ----- Color conversion API -------------------------------------------- */

/* Convert 24-bit RGB to closest 256-color palette index (0-255) */
int bflare_color_rgb_to_256(int r, int g, int b);

/* Convert 24-bit RGB to closest 8-color ANSI index (0-7) */
int bflare_color_rgb_to_8(int r, int g, int b);

/* Convert 24-bit RGB to closest 16-color ANSI index (0-15) via nearest-match */
int bflare_color_rgb_to_16(int r, int g, int b);

/* Reverse mapping: 256-color palette index to RGB */
void bflare_color_256_to_rgb(int idx, uint8_t *r, uint8_t *g, uint8_t *b);

/* ----- One-shot highlight API ------------------------------------------ */

/* Highlight source text in one call.
 *
 * lexer, style, formatter are all optional (NULL = defaults:
 * bloom-lisp lexer, monokai style, truecolor terminal formatter).
 *
 * Returns a BflareResult with a malloc'd ANSI string.
 * Caller frees with bflare_result_free(). */
BflareResult bflare_highlight(const char *input, size_t input_len,
                              const BflareLexer *lexer,
                              const BflareStyle *style,
                              const BflareFormatter *formatter);

void bflare_result_free(BflareResult result);

#endif /* BLOOM_FLARE_HIGHLIGHT_H */

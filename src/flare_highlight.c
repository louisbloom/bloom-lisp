/* highlight.c - One-shot bflare_highlight() convenience */

#include "../include/bloom-lisp/highlight.h"
#include "../include/lisp.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations from other source files */
extern BflareColorDepth bflare_formatter_depth(const BflareFormatter *f);
extern char *bflare_format_terminal(const BflareToken *tokens, size_t count,
                                    const char *input, const BflareStyle *style,
                                    BflareColorDepth depth);

BflareResult bflare_highlight(const char *input, size_t input_len,
                              const BflareLexer *lexer,
                              const BflareStyle *style,
                              const BflareFormatter *formatter)
{
    BflareResult result = { NULL, 0 };

    if (!input)
        input = "";
    if (input_len == 0 && input[0] != '\0')
        input_len = 0;

    /* Resolve defaults */
    BflareLexer *def_lexer = NULL;
    Environment *def_env = NULL;
    if (!lexer) {
        def_env = lisp_init();
        def_lexer = bflare_lexer_bloom_lisp(def_env);
        lexer = def_lexer;
    }

    BflareStyle *def_style = NULL;
    if (!style) {
        def_style = bflare_style_dracula();
        style = def_style;
    }

    BflareFormatter *def_fmt = NULL;
    if (!formatter) {
        def_fmt = bflare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
        formatter = def_fmt;
    }

    /* Lex */
    size_t token_count = 0;
    BflareToken *tokens = bflare_lex(lexer, input,
                                     input_len > 0 ? input_len : strlen(input),
                                     &token_count);

    /* Format */
    BflareColorDepth depth = bflare_formatter_depth(formatter);
    char *ansi = bflare_format_terminal(tokens, token_count, input, style, depth);

    /* Clean up tokens */
    free(tokens);

    /* Clean up defaults */
    if (def_lexer)
        bflare_lexer_free(def_lexer);
    if (def_env)
        lisp_cleanup();
    if (def_style)
        bflare_style_free(def_style);
    if (def_fmt)
        bflare_formatter_free(def_fmt);

    result.data = ansi;
    result.length = ansi ? strlen(ansi) : 0;
    return result;
}

void bflare_result_free(BflareResult result)
{
    free(result.data);
}

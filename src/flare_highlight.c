/* highlight.c - One-shot flare_highlight() convenience */

#include "../include/ditty/highlight.h"
#include "../include/lisp.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations from other source files */
extern FlareColorDepth flare_formatter_depth(const FlareFormatter *f);
extern char *flare_format_terminal(const FlareToken *tokens, size_t count,
                                   const char *input, const FlareStyle *style,
                                   FlareColorDepth depth);

FlareResult flare_highlight(const char *input, size_t input_len,
                            const FlareLexer *lexer,
                            const FlareStyle *style,
                            const FlareFormatter *formatter)
{
    FlareResult result = { NULL, 0 };

    if (!input)
        input = "";
    if (input_len == 0 && input[0] != '\0')
        input_len = 0;

    /* Resolve defaults */
    FlareLexer *def_lexer = NULL;
    Environment *def_env = NULL;
    if (!lexer) {
        def_env = lisp_init();
        def_lexer = flare_lexer_ditty(def_env);
        lexer = def_lexer;
    }

    FlareStyle *def_style = NULL;
    if (!style) {
        def_style = flare_style_dracula();
        style = def_style;
    }

    FlareFormatter *def_fmt = NULL;
    if (!formatter) {
        def_fmt = flare_formatter_terminal(BFLARE_COLOR_TRUECOLOR);
        formatter = def_fmt;
    }

    /* Lex */
    size_t token_count = 0;
    FlareToken *tokens = flare_lex(lexer, input,
                                   input_len > 0 ? input_len : strlen(input),
                                   &token_count);

    /* Format */
    FlareColorDepth depth = flare_formatter_depth(formatter);
    char *ansi = flare_format_terminal(tokens, token_count, input, style, depth);

    /* Clean up tokens */
    free(tokens);

    /* Clean up defaults */
    if (def_lexer)
        flare_lexer_free(def_lexer);
    if (def_env)
        lisp_cleanup();
    if (def_style)
        flare_style_free(def_style);
    if (def_fmt)
        flare_formatter_free(def_fmt);

    result.data = ansi;
    result.length = ansi ? strlen(ansi) : 0;
    return result;
}

void flare_result_free(FlareResult result)
{
    free(result.data);
}

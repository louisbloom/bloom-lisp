/* style_github_dark.c - Built-in GitHub Dark style */

#include "../include/bloom-lisp/highlight.h"
#include <stdlib.h>

BflareStyle *bflare_style_build_github_dark(void)
{
    BflareStyle *s = bflare_style_new();
    if (!s)
        return NULL;

    BflareStyleEntry e;

    e = (BflareStyleEntry){ .fg_r = 201, .fg_g = 209, .fg_b = 217, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_TEXT, &e);

    e = (BflareStyleEntry){ .fg_r = 255, .fg_g = 123, .fg_b = 114, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_KEYWORD, &e);

    e = (BflareStyleEntry){ .fg_r = 210, .fg_g = 168, .fg_b = 255, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_FUNCTION, &e);

    e = (BflareStyleEntry){ .fg_r = 121, .fg_g = 192, .fg_b = 255, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_BUILTIN, &e);

    e = (BflareStyleEntry){ .fg_r = 255, .fg_g = 123, .fg_b = 114, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_KEYWORD_ARG, &e);

    e = (BflareStyleEntry){ .fg_r = 166, .fg_g = 218, .fg_b = 149, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_STRING, &e);

    e = (BflareStyleEntry){ .fg_r = 121, .fg_g = 192, .fg_b = 255, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_NUMBER, &e);

    e = (BflareStyleEntry){ .fg_r = 128, .fg_g = 128, .fg_b = 128, .bold = 0, .italic = 0, .underline = 0, .faint = 1, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_NIL, &e);

    e = (BflareStyleEntry){ .fg_r = 248, .fg_g = 248, .fg_b = 242, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_OPERATOR, &e);

    e = (BflareStyleEntry){ .fg_r = 201, .fg_g = 209, .fg_b = 217, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_PUNCTUATION, &e);

    e = (BflareStyleEntry){ .fg_r = 139, .fg_g = 148, .fg_b = 158, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_COMMENT, &e);

    e = (BflareStyleEntry){ .fg_r = 255, .fg_g = 70, .fg_b = 70, .bold = 0, .italic = 0, .underline = 1, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_ERROR, &e);

    return s;
}

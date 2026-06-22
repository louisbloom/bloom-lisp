/* style_github_light.c - Built-in GitHub Light style */

#include "../include/bloom-lisp/highlight.h"
#include <stdlib.h>

BflareStyle *bflare_style_build_github_light(void)
{
    BflareStyle *s = bflare_style_new();
    if (!s)
        return NULL;

    BflareStyleEntry e;

    e = (BflareStyleEntry){ .fg_r = 36, .fg_g = 41, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_TEXT, &e);

    e = (BflareStyleEntry){ .fg_r = 215, .fg_g = 58, .fg_b = 73, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_KEYWORD, &e);

    e = (BflareStyleEntry){ .fg_r = 111, .fg_g = 66, .fg_b = 193, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_FUNCTION, &e);

    e = (BflareStyleEntry){ .fg_r = 0, .fg_g = 92, .fg_b = 197, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_BUILTIN, &e);

    e = (BflareStyleEntry){ .fg_r = 215, .fg_g = 58, .fg_b = 73, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_KEYWORD_ARG, &e);

    e = (BflareStyleEntry){ .fg_r = 47, .fg_g = 139, .fg_b = 87, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_STRING, &e);

    e = (BflareStyleEntry){ .fg_r = 0, .fg_g = 92, .fg_b = 197, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_NUMBER, &e);

    e = (BflareStyleEntry){ .fg_r = 128, .fg_g = 128, .fg_b = 128, .bold = 0, .italic = 0, .underline = 0, .faint = 1, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_NIL, &e);

    e = (BflareStyleEntry){ .fg_r = 36, .fg_g = 41, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_OPERATOR, &e);

    e = (BflareStyleEntry){ .fg_r = 36, .fg_g = 41, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_PUNCTUATION, &e);

    e = (BflareStyleEntry){ .fg_r = 106, .fg_g = 115, .fg_b = 125, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_COMMENT, &e);

    e = (BflareStyleEntry){ .fg_r = 215, .fg_g = 58, .fg_b = 73, .bold = 0, .italic = 0, .underline = 1, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_ERROR, &e);

    return s;
}

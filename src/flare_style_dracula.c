/* style_dracula.c - Built-in Dracula style */

#include "../include/bloom-lisp/highlight.h"
#include <stdlib.h>

BflareStyle *bflare_style_build_dracula(void)
{
    BflareStyle *s = bflare_style_new();
    if (!s)
        return NULL;

    BflareStyleEntry e;

    e = (BflareStyleEntry){ .fg_r = 248, .fg_g = 248, .fg_b = 242, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_TEXT, &e);

    e = (BflareStyleEntry){ .fg_r = 255, .fg_g = 121, .fg_b = 198, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_KEYWORD, &e);

    e = (BflareStyleEntry){ .fg_r = 255, .fg_g = 6, .fg_b = 183, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_KEYWORD_DEFINE, &e);

    e = (BflareStyleEntry){ .fg_r = 189, .fg_g = 147, .fg_b = 249, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_FUNCTION, &e);

    e = (BflareStyleEntry){ .fg_r = 139, .fg_g = 233, .fg_b = 253, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_BUILTIN, &e);

    e = (BflareStyleEntry){ .fg_r = 248, .fg_g = 248, .fg_b = 242, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_VARIABLE, &e);

    e = (BflareStyleEntry){ .fg_r = 255, .fg_g = 121, .fg_b = 198, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_NAME_KEYWORD_ARG, &e);

    e = (BflareStyleEntry){ .fg_r = 241, .fg_g = 250, .fg_b = 140, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_STRING, &e);

    e = (BflareStyleEntry){ .fg_r = 189, .fg_g = 147, .fg_b = 249, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_NUMBER, &e);

    e = (BflareStyleEntry){ .fg_r = 128, .fg_g = 128, .fg_b = 128, .bold = 0, .italic = 0, .underline = 0, .faint = 1, .strikethrough = 0 };
    bflare_style_set(s, HL_LITERAL_NIL, &e);

    e = (BflareStyleEntry){ .fg_r = 255, .fg_g = 121, .fg_b = 198, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_OPERATOR, &e);

    e = (BflareStyleEntry){ .fg_r = 248, .fg_g = 248, .fg_b = 242, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_PUNCTUATION, &e);

    e = (BflareStyleEntry){ .fg_r = 98, .fg_g = 114, .fg_b = 164, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_COMMENT, &e);

    e = (BflareStyleEntry){ .fg_r = 255, .fg_g = 70, .fg_b = 70, .bold = 0, .italic = 0, .underline = 1, .faint = 0, .strikethrough = 0 };
    bflare_style_set(s, HL_ERROR, &e);

    return s;
}

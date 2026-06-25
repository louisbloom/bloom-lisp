/* style_github_light.c - Built-in GitHub Light style */

#include "../include/bloom-lisp/highlight.h"
#include <stdlib.h>

FlareStyle *flare_style_build_github_light(void)
{
    FlareStyle *s = flare_style_new();
    if (!s)
        return NULL;

    FlareStyleEntry e;

    e = (FlareStyleEntry){ .fg_r = 36, .fg_g = 41, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_TEXT, &e);

    e = (FlareStyleEntry){ .fg_r = 215, .fg_g = 58, .fg_b = 73, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_KEYWORD, &e);

    e = (FlareStyleEntry){ .fg_r = 111, .fg_g = 66, .fg_b = 193, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_NAME_FUNCTION, &e);

    e = (FlareStyleEntry){ .fg_r = 0, .fg_g = 92, .fg_b = 197, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_NAME_BUILTIN, &e);

    e = (FlareStyleEntry){ .fg_r = 215, .fg_g = 58, .fg_b = 73, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_NAME_KEYWORD_ARG, &e);

    e = (FlareStyleEntry){ .fg_r = 47, .fg_g = 139, .fg_b = 87, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_LITERAL_STRING, &e);

    e = (FlareStyleEntry){ .fg_r = 0, .fg_g = 92, .fg_b = 197, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_LITERAL_NUMBER, &e);

    e = (FlareStyleEntry){ .fg_r = 128, .fg_g = 128, .fg_b = 128, .bold = 0, .italic = 0, .underline = 0, .faint = 1, .strikethrough = 0 };
    flare_style_set(s, HL_LITERAL_NIL, &e);

    e = (FlareStyleEntry){ .fg_r = 36, .fg_g = 41, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_OPERATOR, &e);

    e = (FlareStyleEntry){ .fg_r = 36, .fg_g = 41, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_PUNCTUATION, &e);

    e = (FlareStyleEntry){ .fg_r = 106, .fg_g = 115, .fg_b = 125, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_COMMENT, &e);

    e = (FlareStyleEntry){ .fg_r = 215, .fg_g = 58, .fg_b = 73, .bold = 0, .italic = 0, .underline = 1, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_ERROR, &e);

    /* Markdown/CommonMark block markup */
    e = (FlareStyleEntry){ .fg_r = 106, .fg_g = 115, .fg_b = 125, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP, &e);

    e = (FlareStyleEntry){ .fg_r = 5, .fg_g = 80, .fg_b = 174, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_HEADING_MARKER, &e);

    e = (FlareStyleEntry){ .fg_r = 5, .fg_g = 80, .fg_b = 174, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_SETEXT_UNDERLINE, &e);

    e = (FlareStyleEntry){ .fg_r = 106, .fg_g = 115, .fg_b = 125, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_FENCED_INFO, &e);

    e = (FlareStyleEntry){ .fg_r = 47, .fg_g = 139, .fg_b = 87, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INDENTED_CODE, &e);

    /* Markdown/CommonMark inline markup */
    e = (FlareStyleEntry){ .fg_r = 215, .fg_g = 58, .fg_b = 73, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE, &e);

    e = (FlareStyleEntry){ .fg_r = 36, .fg_g = 41, .fg_b = 46, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_EMPHASIS, &e);

    e = (FlareStyleEntry){ .fg_r = 36, .fg_g = 41, .fg_b = 46, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_STRONG, &e);

    e = (FlareStyleEntry){ .fg_r = 47, .fg_g = 139, .fg_b = 87, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_CODE, &e);

    e = (FlareStyleEntry){ .fg_r = 0, .fg_g = 92, .fg_b = 197, .bold = 0, .italic = 0, .underline = 1, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_LINK, &e);

    e = (FlareStyleEntry){ .fg_r = 0, .fg_g = 92, .fg_b = 197, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_IMAGE, &e);

    e = (FlareStyleEntry){ .fg_r = 0, .fg_g = 92, .fg_b = 197, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_AUTOLINK, &e);

    e = (FlareStyleEntry){ .fg_r = 106, .fg_g = 115, .fg_b = 125, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_BREAK, &e);

    e = (FlareStyleEntry){ .fg_r = 47, .fg_g = 139, .fg_b = 87, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_ESCAPE, &e);

    e = (FlareStyleEntry){ .fg_r = 47, .fg_g = 139, .fg_b = 87, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_ENTITY, &e);

    e = (FlareStyleEntry){ .fg_r = 106, .fg_g = 115, .fg_b = 125, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_HTML, &e);

    return s;
}

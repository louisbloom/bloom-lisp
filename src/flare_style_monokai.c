/* style_monokai.c - Built-in Monokai style */

#include "../include/ditty/highlight.h"
#include <stdlib.h>

FlareStyle *flare_style_build_monokai(void)
{
    FlareStyle *s = flare_style_new();
    if (!s)
        return NULL;

    FlareStyleEntry e;

    /* Text / default */
    e = (FlareStyleEntry){ .fg_r = 248, .fg_g = 248, .fg_b = 248, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_TEXT, &e);

    /* Keywords */
    e = (FlareStyleEntry){ .fg_r = 102, .fg_g = 217, .fg_b = 239, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_KEYWORD, &e);

    /* Names */
    e = (FlareStyleEntry){ .fg_r = 255, .fg_g = 215, .fg_b = 0, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_NAME, &e);

    e = (FlareStyleEntry){ .fg_r = 166, .fg_g = 226, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_NAME_FUNCTION, &e);

    e = (FlareStyleEntry){ .fg_r = 118, .fg_g = 171, .fg_b = 210, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_NAME_BUILTIN, &e);

    e = (FlareStyleEntry){ .fg_r = 255, .fg_g = 215, .fg_b = 0, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_NAME_VARIABLE, &e);

    e = (FlareStyleEntry){ .fg_r = 255, .fg_g = 117, .fg_b = 198, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_NAME_KEYWORD_ARG, &e);

    /* Literals */
    e = (FlareStyleEntry){ .fg_r = 230, .fg_g = 219, .fg_b = 116, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_LITERAL_STRING, &e);

    e = (FlareStyleEntry){ .fg_r = 174, .fg_g = 129, .fg_b = 255, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_LITERAL_NUMBER, &e);

    e = (FlareStyleEntry){ .fg_r = 174, .fg_g = 129, .fg_b = 255, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_LITERAL_CHARACTER, &e);

    e = (FlareStyleEntry){ .fg_r = 174, .fg_g = 129, .fg_b = 255, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_LITERAL_BOOLEAN, &e);

    e = (FlareStyleEntry){ .fg_r = 128, .fg_g = 128, .fg_b = 128, .bold = 0, .italic = 0, .underline = 0, .faint = 1, .strikethrough = 0 };
    flare_style_set(s, HL_LITERAL_NIL, &e);

    /* Operators */
    e = (FlareStyleEntry){ .fg_r = 255, .fg_g = 117, .fg_b = 198, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_OPERATOR, &e);

    /* Punctuation */
    e = (FlareStyleEntry){ .fg_r = 248, .fg_g = 248, .fg_b = 248, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_PUNCTUATION, &e);

    /* Comments */
    e = (FlareStyleEntry){ .fg_r = 117, .fg_g = 113, .fg_b = 94, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_COMMENT, &e);

    /* Errors */
    e = (FlareStyleEntry){ .fg_r = 255, .fg_g = 70, .fg_b = 70, .bold = 0, .italic = 0, .underline = 1, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_ERROR, &e);

    /* Markdown/CommonMark block markup */
    e = (FlareStyleEntry){ .fg_r = 117, .fg_g = 113, .fg_b = 94, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP, &e);

    e = (FlareStyleEntry){ .fg_r = 102, .fg_g = 217, .fg_b = 239, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_HEADING_MARKER, &e);

    e = (FlareStyleEntry){ .fg_r = 102, .fg_g = 217, .fg_b = 239, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_SETEXT_UNDERLINE, &e);

    e = (FlareStyleEntry){ .fg_r = 117, .fg_g = 113, .fg_b = 94, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_FENCED_INFO, &e);

    e = (FlareStyleEntry){ .fg_r = 230, .fg_g = 219, .fg_b = 116, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INDENTED_CODE, &e);

    /* Markdown/CommonMark inline markup */
    e = (FlareStyleEntry){ .fg_r = 255, .fg_g = 117, .fg_b = 198, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE, &e);

    e = (FlareStyleEntry){ .fg_r = 248, .fg_g = 248, .fg_b = 248, .bold = 0, .italic = 1, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_EMPHASIS, &e);

    e = (FlareStyleEntry){ .fg_r = 248, .fg_g = 248, .fg_b = 248, .bold = 1, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_STRONG, &e);

    e = (FlareStyleEntry){ .fg_r = 230, .fg_g = 219, .fg_b = 116, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_CODE, &e);

    e = (FlareStyleEntry){ .fg_r = 166, .fg_g = 226, .fg_b = 46, .bold = 0, .italic = 0, .underline = 1, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_LINK, &e);

    e = (FlareStyleEntry){ .fg_r = 166, .fg_g = 226, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_IMAGE, &e);

    e = (FlareStyleEntry){ .fg_r = 166, .fg_g = 226, .fg_b = 46, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_AUTOLINK, &e);

    e = (FlareStyleEntry){ .fg_r = 117, .fg_g = 113, .fg_b = 94, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_BREAK, &e);

    e = (FlareStyleEntry){ .fg_r = 230, .fg_g = 219, .fg_b = 116, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_ESCAPE, &e);

    e = (FlareStyleEntry){ .fg_r = 230, .fg_g = 219, .fg_b = 116, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_ENTITY, &e);

    e = (FlareStyleEntry){ .fg_r = 117, .fg_g = 113, .fg_b = 94, .bold = 0, .italic = 0, .underline = 0, .faint = 0, .strikethrough = 0 };
    flare_style_set(s, HL_MARKUP_INLINE_HTML, &e);

    return s;
}

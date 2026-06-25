/* token_type.c - Token type hierarchy and helpers */

#include "../include/bloom-lisp/highlight.h"

int flare_token_category(FlareTokenType type)
{
    if (type < 1000)
        return 0;
    if (type < 2000)
        return 1000;
    if (type < 3000)
        return 2000;
    if (type < 4000)
        return 3000;
    if (type < 5000)
        return 4000;
    if (type < 6000)
        return 5000;
    if (type < 7000)
        return 6000;
    if (type < 8000)
        return 7000;
    if (type < 9000)
        return 8000;
    return 9000;
}

int flare_token_subcategory(FlareTokenType type)
{
    int cat = flare_token_category(type);
    if (type == cat)
        return cat;
    /* Round down to the nearest 10 within the category's range */
    return cat + ((type - cat) / 10) * 10;
}

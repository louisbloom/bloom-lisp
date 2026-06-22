/* formatter.c - BflareFormatter interface */

#include "../include/bloom-lisp/highlight.h"
#include <stdlib.h>

struct BflareFormatter
{
    BflareColorDepth depth;
};

BflareFormatter *bflare_formatter_terminal(BflareColorDepth depth)
{
    BflareFormatter *f = malloc(sizeof(BflareFormatter));
    if (!f)
        return NULL;
    f->depth = depth;
    return f;
}

void bflare_formatter_free(BflareFormatter *formatter)
{
    free(formatter);
}

/* Internal accessors used by formatter_terminal.c */
BflareColorDepth bflare_formatter_depth(const BflareFormatter *f)
{
    return f ? f->depth : BFLARE_COLOR_TRUECOLOR;
}

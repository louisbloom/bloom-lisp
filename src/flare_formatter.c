/* formatter.c - FlareFormatter interface */

#include "../include/ditty/highlight.h"
#include <stdlib.h>

struct FlareFormatter
{
    FlareColorDepth depth;
};

FlareFormatter *flare_formatter_terminal(FlareColorDepth depth)
{
    FlareFormatter *f = malloc(sizeof(FlareFormatter));
    if (!f)
        return NULL;
    f->depth = depth;
    return f;
}

void flare_formatter_free(FlareFormatter *formatter)
{
    free(formatter);
}

/* Internal accessors used by formatter_terminal.c */
FlareColorDepth flare_formatter_depth(const FlareFormatter *f)
{
    return f ? f->depth : BFLARE_COLOR_TRUECOLOR;
}

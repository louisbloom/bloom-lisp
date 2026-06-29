/* style.c - FlareStyle hierarchy resolution */

#include "../include/ditty/highlight.h"
#include <stdlib.h>
#include <string.h>

#define MAX_STYLE_ENTRIES 96

typedef struct
{
    FlareTokenType type;
    FlareStyleEntry entry;
} StyleMapping;

struct FlareStyle
{
    StyleMapping entries[MAX_STYLE_ENTRIES];
    size_t count;
};

static const FlareStyleEntry EMPTY_ENTRY = { 0 };

FlareStyle *flare_style_new(void)
{
    FlareStyle *s = calloc(1, sizeof(FlareStyle));
    return s;
}

void flare_style_free(FlareStyle *style)
{
    free(style);
}

void flare_style_set(FlareStyle *style, FlareTokenType type,
                     const FlareStyleEntry *entry)
{
    if (!style || !entry)
        return;
    /* Replace existing entry for this type */
    for (size_t i = 0; i < style->count; i++) {
        if (style->entries[i].type == type) {
            style->entries[i].entry = *entry;
            return;
        }
    }
    /* Add new entry */
    if (style->count < MAX_STYLE_ENTRIES) {
        style->entries[style->count].type = type;
        style->entries[style->count].entry = *entry;
        style->count++;
    }
}

/* Find an explicit entry for a given type (no hierarchy walk) */
static const FlareStyleEntry *find_entry(const FlareStyle *style, FlareTokenType type)
{
    for (size_t i = 0; i < style->count; i++) {
        if (style->entries[i].type == type)
            return &style->entries[i].entry;
    }
    return NULL;
}

FlareStyleEntry flare_style_get(const FlareStyle *style, FlareTokenType type)
{
    if (!style)
        return EMPTY_ENTRY;

    /* Walk hierarchy: specific → subcategory → category → HL_TEXT */
    const FlareStyleEntry *e;

    e = find_entry(style, type);
    if (e)
        return *e;

    int sub = flare_token_subcategory(type);
    if (sub != type) {
        e = find_entry(style, sub);
        if (e)
            return *e;
    }

    int cat = flare_token_category(type);
    if (cat != sub) {
        e = find_entry(style, cat);
        if (e)
            return *e;
    }

    if (cat != 0) {
        e = find_entry(style, HL_TEXT);
        if (e)
            return *e;
    }

    return EMPTY_ENTRY;
}

/* Helper to add an RGB+attrs entry to a style being built */
static void add_rgb(FlareStyle *s, FlareTokenType type,
                    int r, int g, int b, int bold, int italic, int underline,
                    int faint, int strikethrough)
{
    FlareStyleEntry e = {
        .fg_r = r, .fg_g = g, .fg_b = b, .bg_r = 0, .bg_g = 0, .bg_b = 0, .bold = bold, .italic = italic, .underline = underline, .faint = faint, .strikethrough = strikethrough
    };
    flare_style_set(s, type, &e);
}

/* Forward declarations for built-in style constructors (defined in their own .c files) */
extern FlareStyle *flare_style_build_monokai(void);
extern FlareStyle *flare_style_build_dracula(void);
extern FlareStyle *flare_style_build_github_dark(void);
extern FlareStyle *flare_style_build_github_light(void);

FlareStyle *flare_style_monokai(void) { return flare_style_build_monokai(); }
FlareStyle *flare_style_dracula(void) { return flare_style_build_dracula(); }
FlareStyle *flare_style_github_dark(void) { return flare_style_build_github_dark(); }
FlareStyle *flare_style_github_light(void) { return flare_style_build_github_light(); }

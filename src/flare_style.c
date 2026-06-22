/* style.c - BflareStyle hierarchy resolution */

#include "../include/bloom-lisp/highlight.h"
#include <stdlib.h>
#include <string.h>

#define MAX_STYLE_ENTRIES 64

typedef struct
{
    BflareTokenType type;
    BflareStyleEntry entry;
} StyleMapping;

struct BflareStyle
{
    StyleMapping entries[MAX_STYLE_ENTRIES];
    size_t count;
};

static const BflareStyleEntry EMPTY_ENTRY = { 0 };

BflareStyle *bflare_style_new(void)
{
    BflareStyle *s = calloc(1, sizeof(BflareStyle));
    return s;
}

void bflare_style_free(BflareStyle *style)
{
    free(style);
}

void bflare_style_set(BflareStyle *style, BflareTokenType type,
                      const BflareStyleEntry *entry)
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
static const BflareStyleEntry *find_entry(const BflareStyle *style, BflareTokenType type)
{
    for (size_t i = 0; i < style->count; i++) {
        if (style->entries[i].type == type)
            return &style->entries[i].entry;
    }
    return NULL;
}

BflareStyleEntry bflare_style_get(const BflareStyle *style, BflareTokenType type)
{
    if (!style)
        return EMPTY_ENTRY;

    /* Walk hierarchy: specific → subcategory → category → HL_TEXT */
    const BflareStyleEntry *e;

    e = find_entry(style, type);
    if (e)
        return *e;

    int sub = bflare_token_subcategory(type);
    if (sub != type) {
        e = find_entry(style, sub);
        if (e)
            return *e;
    }

    int cat = bflare_token_category(type);
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
static void add_rgb(BflareStyle *s, BflareTokenType type,
                    int r, int g, int b, int bold, int italic, int underline,
                    int faint, int strikethrough)
{
    BflareStyleEntry e = {
        .fg_r = r, .fg_g = g, .fg_b = b, .bg_r = 0, .bg_g = 0, .bg_b = 0, .bold = bold, .italic = italic, .underline = underline, .faint = faint, .strikethrough = strikethrough
    };
    bflare_style_set(s, type, &e);
}

/* Forward declarations for built-in style constructors (defined in their own .c files) */
extern BflareStyle *bflare_style_build_monokai(void);
extern BflareStyle *bflare_style_build_dracula(void);
extern BflareStyle *bflare_style_build_github_dark(void);
extern BflareStyle *bflare_style_build_github_light(void);

BflareStyle *bflare_style_monokai(void) { return bflare_style_build_monokai(); }
BflareStyle *bflare_style_dracula(void) { return bflare_style_build_dracula(); }
BflareStyle *bflare_style_github_dark(void) { return bflare_style_build_github_dark(); }
BflareStyle *bflare_style_github_light(void) { return bflare_style_build_github_light(); }

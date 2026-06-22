/* formatter_terminal.c - ANSI terminal formatter */

#include "../include/bloom-lisp/highlight.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Format an SGR fg color sequence into buf. Returns bytes written. */
static int format_fg_color(char *buf, size_t bufsize, const BflareStyleEntry *entry,
                           BflareColorDepth depth)
{
    if (depth == BFLARE_COLOR_TRUECOLOR) {
        return snprintf(buf, bufsize, "\033[38;2;%d;%d;%dm",
                        entry->fg_r, entry->fg_g, entry->fg_b);
    }
    if (depth == BFLARE_COLOR_256) {
        int idx = bflare_color_rgb_to_256(entry->fg_r, entry->fg_g, entry->fg_b);
        return snprintf(buf, bufsize, "\033[38;5;%dm", idx);
    }
    /* 16-color: aixterm bright codes 90-97 */
    if (depth == BFLARE_COLOR_16) {
        int idx = bflare_color_rgb_to_16(entry->fg_r, entry->fg_g, entry->fg_b);
        if (idx >= 8)
            return snprintf(buf, bufsize, "\033[%dm", 90 + (idx - 8));
        return snprintf(buf, bufsize, "\033[%dm", 30 + idx);
    }
    /* 8-color: bold prefix for bright */
    int idx = bflare_color_rgb_to_8(entry->fg_r, entry->fg_g, entry->fg_b);
    if (idx >= 8)
        return snprintf(buf, bufsize, "\033[1m\033[%dm", 30 + (idx - 8));
    return snprintf(buf, bufsize, "\033[%dm", 30 + idx);
}

/* Check if two style entries are visually identical */
static int style_entries_equal(const BflareStyleEntry *a, const BflareStyleEntry *b)
{
    return a->fg_r == b->fg_r && a->fg_g == b->fg_g && a->fg_b == b->fg_b &&
           a->bg_r == b->bg_r && a->bg_g == b->bg_g && a->bg_b == b->bg_b &&
           a->bold == b->bold && a->italic == b->italic &&
           a->underline == b->underline && a->faint == b->faint &&
           a->strikethrough == b->strikethrough;
}

/* Format token stream into an ANSI string */
char *bflare_format_terminal(const BflareToken *tokens, size_t count,
                             const char *input, const BflareStyle *style,
                             BflareColorDepth depth)
{
    if (!tokens || count == 0 || !input || !style) {
        char *empty = malloc(5);
        if (empty)
            memcpy(empty, "\033[0m", 4), empty[4] = '\0';
        return empty;
    }

    /* Grow-only buffer */
    size_t cap = 256;
    char *out = malloc(cap);
    size_t pos = 0;

    BflareStyleEntry prev = { 0 };
    int prev_valid = 0;

    for (size_t i = 0; i < count; i++) {
        BflareStyleEntry entry = bflare_style_get(style, tokens[i].type);

        /* Coalesce: if same style as previous, skip the escape */
        if (prev_valid && style_entries_equal(&prev, &entry)) {
            /* Just copy the text */
            size_t tlen = tokens[i].length;
            while (pos + tlen + 5 > cap) {
                cap *= 2;
                out = realloc(out, cap);
            }
            memcpy(out + pos, input + tokens[i].offset, tlen);
            pos += tlen;
            continue;
        }

        /* Build SGR sequence for this style */
        char sgr[128];
        int sglen = 0;

        /* Start building the CSI sequence */
        int need_semi = 0;
        int sgrpos = 0;
        sgr[sgrpos++] = '\033';
        sgr[sgrpos++] = '[';

        if (entry.bold) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = sprintf(sgr + sgrpos, "1");
            sgrpos += sglen;
            need_semi = 1;
        }
        if (entry.faint) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = sprintf(sgr + sgrpos, "2");
            sgrpos += sglen;
            need_semi = 1;
        }
        if (entry.italic) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = sprintf(sgr + sgrpos, "3");
            sgrpos += sglen;
            need_semi = 1;
        }
        if (entry.underline) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = sprintf(sgr + sgrpos, "4");
            sgrpos += sglen;
            need_semi = 1;
        }
        if (entry.strikethrough) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            sglen = sprintf(sgr + sgrpos, "9");
            sgrpos += sglen;
            need_semi = 1;
        }

        /* Foreground color */
        char fgbuf[32];
        int fglen;
        if (depth == BFLARE_COLOR_TRUECOLOR) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            fglen = sprintf(fgbuf, "38;2;%d;%d;%d", entry.fg_r, entry.fg_g, entry.fg_b);
            memcpy(sgr + sgrpos, fgbuf, fglen);
            sgrpos += fglen;
            need_semi = 1;
        } else if (depth == BFLARE_COLOR_256) {
            if (need_semi)
                sgr[sgrpos++] = ';';
            int idx = bflare_color_rgb_to_256(entry.fg_r, entry.fg_g, entry.fg_b);
            fglen = sprintf(fgbuf, "38;5;%d", idx);
            memcpy(sgr + sgrpos, fgbuf, fglen);
            sgrpos += fglen;
            need_semi = 1;
        } else if (depth == BFLARE_COLOR_16) {
            /* 16-color: aixterm bright codes 90-97 */
            int idx = bflare_color_rgb_to_16(entry.fg_r, entry.fg_g, entry.fg_b);
            if (need_semi)
                sgr[sgrpos++] = ';';
            if (idx >= 8) {
                fglen = sprintf(fgbuf, "%d", 90 + (idx - 8));
            } else {
                fglen = sprintf(fgbuf, "%d", 30 + idx);
            }
            memcpy(sgr + sgrpos, fgbuf, fglen);
            sgrpos += fglen;
            need_semi = 1;
        } else {
            /* 8-color: normal codes 30-37, bold-intensity for bright */
            int idx = bflare_color_rgb_to_8(entry.fg_r, entry.fg_g, entry.fg_b);
            if (idx >= 8) {
                /* Bright: emit 1; (bold-intensity) + color in one SGR param.
                 * Avoid double-bold if entry.bold was already emitted above. */
                if (!entry.bold) {
                    if (need_semi)
                        sgr[sgrpos++] = ';';
                    fglen = sprintf(fgbuf, "1;%d", 30 + (idx - 8));
                } else {
                    /* Bold was already emitted as "1" — just append color */
                    if (need_semi)
                        sgr[sgrpos++] = ';';
                    fglen = sprintf(fgbuf, "%d", 30 + (idx - 8));
                }
                memcpy(sgr + sgrpos, fgbuf, fglen);
                sgrpos += fglen;
                need_semi = 1;
            } else {
                if (need_semi)
                    sgr[sgrpos++] = ';';
                fglen = sprintf(fgbuf, "%d", 30 + idx);
                memcpy(sgr + sgrpos, fgbuf, fglen);
                sgrpos += fglen;
                need_semi = 1;
            }
        }

        sgr[sgrpos++] = 'm';

        /* Write SGR + token text */
        size_t tlen = tokens[i].length;
        while (pos + sgrpos + tlen + 5 > cap) {
            cap *= 2;
            out = realloc(out, cap);
        }

        memcpy(out + pos, sgr, sgrpos);
        pos += sgrpos;
        memcpy(out + pos, input + tokens[i].offset, tlen);
        pos += tlen;

        prev = entry;
        prev_valid = 1;
    }

    /* Trailing SGR reset */
    while (pos + 5 > cap) {
        cap *= 2;
        out = realloc(out, cap);
    }
    memcpy(out + pos, "\033[0m", 4);
    pos += 4;
    out[pos] = '\0';

    return out;
}

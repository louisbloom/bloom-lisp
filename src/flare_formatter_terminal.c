/* formatter_terminal.c - ANSI terminal formatter */

#include "../include/bloom-lisp/highlight.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Format an SGR fg color sequence into buf. Returns bytes written. */
static int format_fg_color(char *buf, size_t bufsize, const FlareStyleEntry *entry,
                           FlareColorDepth depth)
{
    if (depth == BFLARE_COLOR_TRUECOLOR) {
        return snprintf(buf, bufsize, "\033[38;2;%d;%d;%dm",
                        entry->fg_r, entry->fg_g, entry->fg_b);
    }
    if (depth == BFLARE_COLOR_256) {
        int idx = flare_color_rgb_to_256(entry->fg_r, entry->fg_g, entry->fg_b);
        return snprintf(buf, bufsize, "\033[38;5;%dm", idx);
    }
    /* 16-color: aixterm bright codes 90-97 */
    if (depth == BFLARE_COLOR_16) {
        int idx = flare_color_rgb_to_16(entry->fg_r, entry->fg_g, entry->fg_b);
        if (idx >= 8)
            return snprintf(buf, bufsize, "\033[%dm", 90 + (idx - 8));
        return snprintf(buf, bufsize, "\033[%dm", 30 + idx);
    }
    /* 8-color: bold prefix for bright */
    int idx = flare_color_rgb_to_8(entry->fg_r, entry->fg_g, entry->fg_b);
    if (idx >= 8)
        return snprintf(buf, bufsize, "\033[1m\033[%dm", 30 + (idx - 8));
    return snprintf(buf, bufsize, "\033[%dm", 30 + idx);
}

/* Check if two style entries are visually identical */
static int style_entries_equal(const FlareStyleEntry *a, const FlareStyleEntry *b)
{
    return a->fg_r == b->fg_r && a->fg_g == b->fg_g && a->fg_b == b->fg_b &&
           a->bg_r == b->bg_r && a->bg_g == b->bg_g && a->bg_b == b->bg_b &&
           a->bold == b->bold && a->italic == b->italic &&
           a->underline == b->underline && a->faint == b->faint &&
           a->strikethrough == b->strikethrough;
}

/* Check if a token type is a fenced code block structural marker that
 * should be suppressed from rendered output.  The fence delimiters
 * (``` or ~~~) and info strings are structural, not visual content. */
static int is_fenced_marker(FlareTokenType type)
{
    return type == HL_MARKUP_FENCED_OPEN || type == HL_MARKUP_FENCED_INFO ||
           type == HL_MARKUP_FENCED_CLOSE;
}

/* Ensure buffer has room for `need` bytes beyond `pos`. */
static void buf_ensure(char **buf, size_t *cap, size_t pos, size_t need)
{
    while (pos + need > *cap) {
        *cap *= 2;
        *buf = realloc(*buf, *cap);
    }
}

/* Copy token text to the output buffer, tracking line boundaries.
 * When `indent` is true, prepend 2 spaces at the start of each line
 * (after every \n). Returns the new output position. */
static size_t emit_token_text(char **out, size_t *cap, size_t pos,
                              const char *input, size_t offset, size_t length,
                              int indent, int *at_line_start)
{
    const char *src = input + offset;

    if (!indent) {
        buf_ensure(out, cap, pos, length + 5);
        memcpy(*out + pos, src, length);
        pos += length;
    } else {
        size_t src_pos = 0;
        while (src_pos < length) {
            const char *nl = memchr(src + src_pos, '\n', length - src_pos);
            if (!nl) {
                size_t chunk = length - src_pos;
                buf_ensure(out, cap, pos, chunk + 3);
                memcpy(*out + pos, src + src_pos, chunk);
                pos += chunk;
                break;
            }
            size_t chunk = (size_t)(nl - (src + src_pos)) + 1;
            buf_ensure(out, cap, pos, chunk + 3);
            memcpy(*out + pos, src + src_pos, chunk);
            pos += chunk;
            src_pos += chunk;
            /* Emit indentation after the newline (unless it's the
             * very end of the token — the next token will handle it
             * via the at_line_start mechanism). */
            if (src_pos < length) {
                buf_ensure(out, cap, pos, 3);
                memcpy(*out + pos, "  ", 2);
                pos += 2;
            }
        }
    }

    if (length > 0 && src[length - 1] == '\n')
        *at_line_start = 1;
    else if (length > 0)
        *at_line_start = 0;
    return pos;
}

/* Format token stream into an ANSI string */
char *flare_format_terminal(const FlareToken *tokens, size_t count,
                            const char *input, const FlareStyle *style,
                            FlareColorDepth depth)
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

    FlareStyleEntry prev = { 0 };
    int prev_valid = 0;
    int in_fenced = 0;
    int at_line_start = 1; /* track line boundaries across tokens */

    for (size_t i = 0; i < count; i++) {
        /* Skip fenced code block structural markers — they are not
         * visual content. When skipping, also consume the line-ending
         * newline that belongs to the fence line:
         *   - \n immediately after FENCED_OPEN/INFO: ends the opening
         *     fence line (not a paragraph separator)
         *   - \n immediately before FENCED_CLOSE: ends the last code
         *     content line (not a paragraph separator)
         * The \n BEFORE FENCED_OPEN and AFTER FENCED_CLOSE are
         * paragraph separators and must be kept. */
        if (is_fenced_marker(tokens[i].type)) {
            if (tokens[i].type == HL_MARKUP_FENCED_OPEN ||
                tokens[i].type == HL_MARKUP_FENCED_INFO) {
                in_fenced = 1;
                at_line_start = 1;
                /* Consume trailing \n that ends the fence line */
                if (i + 1 < count && tokens[i + 1].type == HL_TEXT &&
                    tokens[i + 1].length == 1 &&
                    input[tokens[i + 1].offset] == '\n')
                    i++;
            } else if (tokens[i].type == HL_MARKUP_FENCED_CLOSE) {
                in_fenced = 0;
            }
            prev_valid = 0;
            continue;
        }
        if (tokens[i].type == HL_TEXT && tokens[i].length == 1 &&
            input[tokens[i].offset] == '\n' &&
            i + 1 < count && tokens[i + 1].type == HL_MARKUP_FENCED_CLOSE) {
            /* \n before closing fence: part of the fence block, not
             * a paragraph separator — suppress.  Also clear
             * in_fenced so subsequent content is not indented. */
            in_fenced = 0;
            prev_valid = 0;
            continue;
        }

        /* Emit 2-space indentation at the start of each line inside
         * a fenced code block, before the first styled token. */
        if (in_fenced && at_line_start) {
            buf_ensure(&out, &cap, pos, 2);
            memcpy(out + pos, "  ", 2);
            pos += 2;
            at_line_start = 0;
        }

        FlareStyleEntry entry = flare_style_get(style, tokens[i].type);

        /* Coalesce: if same style as previous, skip the escape */
        if (prev_valid && style_entries_equal(&prev, &entry)) {
            /* Just copy the text */
            size_t tlen = tokens[i].length;
            pos = emit_token_text(&out, &cap, pos, input,
                                  tokens[i].offset, tlen, in_fenced,
                                  &at_line_start);
            continue;
        }

        /* Build SGR sequence for this style.
         * Always lead with reset (0) to clear attributes from the previous
         * token — ANSI attributes are cumulative, so without an explicit
         * reset a bold from token N would persist into token N+1. */
        char sgr[128];
        int sglen = 0;

        int need_semi = 0;
        int sgrpos = 0;
        sgr[sgrpos++] = '\033';
        sgr[sgrpos++] = '[';
        sgr[sgrpos++] = '0';
        need_semi = 1;

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
            int idx = flare_color_rgb_to_256(entry.fg_r, entry.fg_g, entry.fg_b);
            fglen = sprintf(fgbuf, "38;5;%d", idx);
            memcpy(sgr + sgrpos, fgbuf, fglen);
            sgrpos += fglen;
            need_semi = 1;
        } else if (depth == BFLARE_COLOR_16) {
            /* 16-color: aixterm bright codes 90-97 */
            int idx = flare_color_rgb_to_16(entry.fg_r, entry.fg_g, entry.fg_b);
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
            int idx = flare_color_rgb_to_8(entry.fg_r, entry.fg_g, entry.fg_b);
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
        buf_ensure(&out, &cap, pos, sgrpos + tlen + 5);

        memcpy(out + pos, sgr, sgrpos);
        pos += sgrpos;
        pos = emit_token_text(&out, &cap, pos, input,
                              tokens[i].offset, tlen, in_fenced,
                              &at_line_start);

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

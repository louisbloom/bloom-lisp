/* lexer_commonmark.c - CommonMark/Markdown scanner
 *
 * Implements the CommonMark spec (v0.31.2) two-phase parsing:
 *   Phase 1 — Block structure: identify block types line-by-line
 *   Phase 2 — Inline structure: tokenize emphasis, links, etc. within text
 *
 * Fenced code blocks with an info string of "lisp", "bloom-lisp",
 * or "bloom" are sub-lexed through the bloom-lisp lexer for
 * syntax highlighting.
 */

#include "../include/bloom-lisp/highlight.h"
#include "../include/lisp.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration from lexer.c */
extern FlareLexer *flare_lexer_alloc(const char *name, void *ctx,
                                     FlareToken *(*scan)(const char *, size_t, void *, size_t *),
                                     void (*free_ctx)(void *));

typedef struct
{
    Environment *env;
} CommonmarkCtx;

/* ----- TokenVec --------------------------------------------------------- */

typedef struct
{
    FlareToken *items;
    size_t count;
    size_t capacity;
} TokenVec;

static void tv_push(TokenVec *v, FlareTokenType type, size_t offset, size_t length)
{
    if (v->count >= v->capacity) {
        v->capacity = v->capacity ? v->capacity * 2 : 64;
        v->items = realloc(v->items, v->capacity * sizeof(FlareToken));
    }
    v->items[v->count].type = type;
    v->items[v->count].offset = offset;
    v->items[v->count].length = length;
    v->count++;
}

static void tv_push_text(TokenVec *v, size_t start, size_t end)
{
    if (end > start)
        tv_push(v, HL_TEXT, start, end - start);
}

/* ----- Line iterator ---------------------------------------------------- */

typedef struct
{
    const char *input;
    size_t input_len;
    size_t pos;
} LineIter;

/* Advance to next line. Returns pointer to line start, sets *line_len.
 * Strips trailing \r. Returns NULL when exhausted. */
static const char *line_next(LineIter *it, size_t *line_len)
{
    if (it->pos >= it->input_len) {
        *line_len = 0;
        return NULL;
    }
    const char *start = it->input + it->pos;
    const char *nl = memchr(start, '\n', it->input_len - it->pos);
    size_t len;
    if (nl) {
        len = (size_t)(nl - start);
        it->pos += len + 1;
    } else {
        len = it->input_len - it->pos;
        it->pos += len;
    }
    if (len > 0 && start[len - 1] == '\r')
        len--;
    *line_len = len;
    return start;
}

static size_t line_offset(const LineIter *it, const char *line_start)
{
    return (size_t)(line_start - it->input);
}

/* ----- Blank line check ------------------------------------------------- */

static int is_blank_line(const char *line, size_t line_len)
{
    for (size_t i = 0; i < line_len; i++) {
        if (line[i] != ' ' && line[i] != '\t')
            return 0;
    }
    return 1;
}

/* ----- Indent counting (tab stop = 4, per spec §2.2) -------------------- */

static size_t count_indent(const char *line, size_t line_len, size_t *out_col)
{
    size_t col = 0;
    size_t i = 0;
    while (i < line_len) {
        if (line[i] == ' ') {
            col++;
            i++;
        } else if (line[i] == '\t') {
            col += 4 - (col % 4);
            i++;
        } else {
            break;
        }
    }
    if (out_col)
        *out_col = col;
    return i;
}

/* ----- Block detection helpers ------------------------------------------ */

/* §4.1 Thematic break: 0-3 spaces indent, then 3+ matching -, _, or *
 * with optional spaces/tabs between. */
static int is_thematic_break(const char *line, size_t line_len)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;
    if (i >= line_len)
        return 0;

    char c = line[i];
    if (c != '-' && c != '_' && c != '*')
        return 0;

    int count = 0;
    while (i < line_len) {
        if (line[i] == c) {
            count++;
            i++;
        } else if (line[i] == ' ' || line[i] == '\t') {
            i++;
        } else {
            return 0;
        }
    }
    return count >= 3;
}

/* §4.2 ATX heading: 0-3 spaces indent, 1-6 #, followed by space/tab/EOL. */
static int is_atx_heading(const char *line, size_t line_len,
                          size_t *out_indent_bytes, size_t *out_marker_len)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;
    if (i >= line_len || line[i] != '#')
        return 0;

    size_t mlen = 0;
    while (i + mlen < line_len && line[i + mlen] == '#' && mlen < 7)
        mlen++;

    if (mlen < 1 || mlen > 6)
        return 0;

    /* Must be followed by space/tab or EOL */
    if (i + mlen < line_len && line[i + mlen] != ' ' && line[i + mlen] != '\t')
        return 0;

    *out_indent_bytes = i;
    *out_marker_len = mlen;
    return 1;
}

/* §4.3 Setext heading underline: 0-3 spaces indent, then 1+ = or -. */
static int is_setext_underline(const char *line, size_t line_len,
                               size_t *out_indent_bytes, size_t *out_underline_len,
                               int *out_level)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;
    if (i >= line_len)
        return 0;

    char c = line[i];
    if (c != '=' && c != '-')
        return 0;

    size_t ulen = 0;
    while (i + ulen < line_len && line[i + ulen] == c)
        ulen++;
    if (ulen < 1)
        return 0;

    /* Only trailing spaces/tabs allowed */
    for (size_t j = i + ulen; j < line_len; j++) {
        if (line[j] != ' ' && line[j] != '\t')
            return 0;
    }

    *out_indent_bytes = i;
    *out_underline_len = ulen;
    *out_level = (c == '=') ? 1 : 2;
    return 1;
}

/* §4.5 Fenced code block opening: 0-3 spaces indent, 3+ ` or ~. */
static int is_opening_fence(const char *line, size_t line_len,
                            size_t *out_indent_bytes, char *out_fence_char,
                            size_t *out_fence_len,
                            size_t *out_info_start, size_t *out_info_len)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;

    if (i >= line_len)
        return 0;

    char fc = line[i];
    if (fc != '`' && fc != '~')
        return 0;

    size_t fl = 0;
    while (i + fl < line_len && line[i + fl] == fc)
        fl++;
    if (fl < 3)
        return 0;

    /* Backtick fences: info string cannot contain backticks */
    size_t istart = i + fl;
    while (istart < line_len && (line[istart] == ' ' || line[istart] == '\t'))
        istart++;

    size_t ilen = line_len - istart;
    while (ilen > 0 && (line[istart + ilen - 1] == ' ' || line[istart + ilen - 1] == '\t'))
        ilen--;

    if (fc == '`') {
        for (size_t j = istart; j < istart + ilen; j++) {
            if (line[j] == '`')
                return 0;
        }
    }

    *out_indent_bytes = i;
    *out_fence_char = fc;
    *out_fence_len = fl;
    *out_info_start = istart;
    *out_info_len = ilen;
    return 1;
}

/* §4.5 Closing fence: 0-3 spaces indent, same char, ≥ opening fence length,
 * only trailing spaces/tabs. */
static int is_closing_fence(const char *line, size_t line_len,
                            char fence_char, size_t open_fence_len)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;
    if (i >= line_len || line[i] != fence_char)
        return 0;

    size_t fl = 0;
    while (i + fl < line_len && line[i + fl] == fence_char)
        fl++;
    if (fl < 3 || fl < open_fence_len)
        return 0;

    for (size_t j = i + fl; j < line_len; j++) {
        if (line[j] != ' ' && line[j] != '\t')
            return 0;
    }
    return 1;
}

/* §4.4 Indented code block: ≥4 spaces indent, non-blank. */
static int is_indented_code(const char *line, size_t line_len,
                            size_t *out_indent_bytes)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col < 4)
        return 0;
    if (i >= line_len || is_blank_line(line, line_len))
        return 0;
    *out_indent_bytes = i;
    return 1;
}

/* §5.1 Block quote: 0-3 spaces indent, then >. */
static int is_block_quote(const char *line, size_t line_len,
                          size_t *out_indent_bytes, size_t *out_marker_len)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;
    if (i >= line_len || line[i] != '>')
        return 0;

    /* Marker is > optionally followed by one space */
    size_t mlen = 1;
    if (i + 1 < line_len && (line[i + 1] == ' ' || line[i + 1] == '\t'))
        mlen = 2;

    *out_indent_bytes = i;
    *out_marker_len = mlen;
    return 1;
}

/* §5.2 List item: 0-3 spaces indent, then bullet (-, +, *) or ordered
 * (1-9 digits + . or )). */
static int is_list_item(const char *line, size_t line_len,
                        size_t *out_indent_bytes, size_t *out_marker_len)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;
    if (i >= line_len)
        return 0;

    /* Bullet list */
    if (line[i] == '-' || line[i] == '+' || line[i] == '*') {
        /* Must be followed by at least one space */
        if (i + 1 >= line_len || (line[i + 1] != ' ' && line[i + 1] != '\t'))
            return 0;
        *out_indent_bytes = i;
        *out_marker_len = 1;
        return 1;
    }

    /* Ordered list: 1-9 digits + . or ) */
    size_t dstart = i;
    size_t dlen = 0;
    while (i + dlen < line_len && isdigit((unsigned char)line[i + dlen]) && dlen < 9)
        dlen++;
    if (dlen < 1)
        return 0;
    if (i + dlen >= line_len)
        return 0;
    if (line[i + dlen] != '.' && line[i + dlen] != ')')
        return 0;
    /* Must be followed by at least one space */
    if (i + dlen + 1 >= line_len || (line[i + dlen + 1] != ' ' && line[i + dlen + 1] != '\t'))
        return 0;

    *out_indent_bytes = i;
    *out_marker_len = dlen + 1;
    return 1;
}

/* §4.6 HTML block start conditions (types 1-7). */
static int is_html_block_start(const char *line, size_t line_len)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;
    if (i >= line_len || line[i] != '<')
        return 0;

    /* Check for start tags we care about (simplified check) */
    static const char *type1_starts[] = { "<pre", "<script", "<style", "<textarea", NULL };
    static const char *type6_tags[] = {
        "address", "article", "aside", "base", "basefont", "blockquote",
        "body", "caption", "center", "col", "colgroup", "dd", "details",
        "dialog", "dir", "div", "dl", "dt", "fieldset", "figcaption",
        "figure", "footer", "form", "frame", "frameset", "h1", "h2", "h3",
        "h4", "h5", "h6", "head", "header", "hr", "html", "iframe",
        "legend", "li", "link", "main", "menu", "menuitem", "nav",
        "noframes", "ol", "optgroup", "option", "p", "param", "search",
        "section", "summary", "table", "tbody", "td", "tfoot", "th",
        "thead", "title", "tr", "track", "ul", NULL
    };

    /* Type 1: <pre, <script, <style, <textarea (case-insensitive) */
    for (int t = 0; type1_starts[t]; t++) {
        size_t tlen = strlen(type1_starts[t]);
        if (i + tlen <= line_len) {
            int match = 1;
            for (size_t k = 0; k < tlen; k++) {
                if (tolower((unsigned char)line[i + k]) != tolower((unsigned char)type1_starts[t][k])) {
                    match = 0;
                    break;
                }
            }
            if (match && (i + tlen >= line_len || line[i + tlen] == ' ' ||
                          line[i + tlen] == '\t' || line[i + tlen] == '>' ||
                          line[i + tlen] == '/'))
                return 1;
        }
    }

    /* Type 2: <!-- */
    if (i + 4 <= line_len && memcmp(line + i, "<!--", 4) == 0)
        return 1;

    /* Type 3: <? */
    if (i + 2 <= line_len && memcmp(line + i, "<?", 2) == 0)
        return 1;

    /* Type 4: <! + ASCII letter */
    if (i + 2 <= line_len && line[i + 1] == '!' && i + 2 < line_len &&
        isalpha((unsigned char)line[i + 2]) && line[i + 2] != '-')
        return 1;

    /* Type 5: <![CDATA[ */
    if (i + 9 <= line_len && memcmp(line + i, "<![CDATA[", 9) == 0)
        return 1;

    /* Type 6: < or </ + block-level tag */
    size_t ti = i + 1;
    int is_closing = 0;
    if (ti < line_len && line[ti] == '/') {
        is_closing = 1;
        ti++;
    }
    for (int t = 0; type6_tags[t]; t++) {
        size_t tlen = strlen(type6_tags[t]);
        if (ti + tlen <= line_len) {
            int match = 1;
            for (size_t k = 0; k < tlen; k++) {
                if (tolower((unsigned char)line[ti + k]) != tolower((unsigned char)type6_tags[t][k])) {
                    match = 0;
                    break;
                }
            }
            if (match && (ti + tlen >= line_len || line[ti + tlen] == ' ' ||
                          line[ti + tlen] == '\t' || line[ti + tlen] == '>' ||
                          line[ti + tlen] == '/'))
                return 1;
        }
    }

    /* Type 7: complete open tag or closing tag on its own line */
    if (ti < line_len && isalpha((unsigned char)line[ti])) {
        size_t j = ti + 1;
        while (j < line_len && (isalnum((unsigned char)line[j]) || line[j] == '-'))
            j++;
        /* Must be followed by attributes then > or />  — simplified check */
        while (j < line_len && line[j] != '>')
            j++;
        if (j < line_len && line[j] == '>') {
            /* Only trailing spaces after > */
            for (size_t k = j + 1; k < line_len; k++) {
                if (line[k] != ' ' && line[k] != '\t')
                    return 0;
            }
            return 1;
        }
    }

    return 0;
}

/* §4.7 Link reference definition: [label]: <url> "title"
 * Simplified detection — looks for leading [ at indent ≤3. */
static int is_link_ref_def(const char *line, size_t line_len,
                           size_t *out_indent_bytes)
{
    size_t col;
    size_t i = count_indent(line, line_len, &col);
    if (col > 3)
        return 0;
    if (i >= line_len || line[i] != '[')
        return 0;

    /* Find matching ]: */
    size_t j = i + 1;
    while (j < line_len && line[j] != ']')
        j++;
    if (j >= line_len)
        return 0;
    if (j + 1 >= line_len || line[j + 1] != ':')
        return 0;

    *out_indent_bytes = i;
    return 1;
}

/* Check if the info string indicates bloom-lisp code */
static int is_lisp_info(const char *line, size_t info_start, size_t info_len)
{
    if (info_len == 0)
        return 0;

    size_t word_end = info_start;
    while (word_end < info_start + info_len && line[word_end] != ' ' &&
           line[word_end] != '\t')
        word_end++;

    size_t word_len = word_end - info_start;

    return (word_len == 4 && strncmp(line + info_start, "lisp", 4) == 0) ||
           (word_len == 10 && strncmp(line + info_start, "bloom-lisp", 10) == 0) ||
           (word_len == 5 && strncmp(line + info_start, "bloom", 5) == 0);
}

/* ----- Inline tokenizer ------------------------------------------------- */

/* Tokenize inline content within a text range.
 * This implements CommonMark §6 inline parsing rules.
 * `base_offset` is added to all emitted token offsets. */
static void tokenize_inlines(TokenVec *v, const char *input, size_t start,
                             size_t end, size_t base_offset)
{
    size_t pos = start;

    while (pos < end) {
        char c = input[pos];

        /* §2.4 Backslash escape: \ before ASCII punctuation */
        if (c == '\\' && pos + 1 < end) {
            char next = input[pos + 1];
            if (isalpha((unsigned char)next) || isdigit((unsigned char)next)) {
                /* Not punctuation — literal backslash */
                pos++;
                continue;
            }
            /* ASCII punctuation: ! " # $ % & ' ( ) * + , - . / : ;
             * < = > ? @ [ \ ] ^ _ ` { | } ~ */
            if (next == '!' || next == '"' || next == '#' || next == '$' ||
                next == '%' || next == '&' || next == '\'' || next == '(' ||
                next == ')' || next == '*' || next == '+' || next == ',' ||
                next == '-' || next == '.' || next == '/' || next == ':' ||
                next == ';' || next == '<' || next == '=' || next == '>' ||
                next == '?' || next == '@' || next == '[' || next == ']' ||
                next == '^' || next == '_' || next == '`' || next == '{' ||
                next == '|' || next == '}' || next == '~' || next == '\\') {
                tv_push(v, HL_MARKUP_INLINE_ESCAPE,
                        base_offset + pos, 2);
                pos += 2;
                continue;
            }
            /* Not before punctuation — literal */
            pos++;
            continue;
        }

        /* §2.5 Entity reference: &...; */
        if (c == '&') {
            size_t amp = pos;
            pos++;
            /* Numeric: &#123; or &#x1F; */
            if (pos < end && input[pos] == '#') {
                pos++;
                if (pos < end && (input[pos] == 'x' || input[pos] == 'X')) {
                    pos++;
                    size_t hex_start = pos;
                    while (pos < end && isxdigit((unsigned char)input[pos]))
                        pos++;
                    if (pos > hex_start && pos < end && input[pos] == ';' && pos - hex_start <= 6) {
                        tv_push(v, HL_MARKUP_INLINE_ENTITY,
                                base_offset + amp, pos + 1 - amp);
                        pos++;
                        continue;
                    }
                } else {
                    size_t dec_start = pos;
                    while (pos < end && isdigit((unsigned char)input[pos]))
                        pos++;
                    if (pos > dec_start && pos < end && input[pos] == ';' && pos - dec_start <= 7) {
                        tv_push(v, HL_MARKUP_INLINE_ENTITY,
                                base_offset + amp, pos + 1 - amp);
                        pos++;
                        continue;
                    }
                }
            } else {
                /* Named entity: &name; */
                size_t name_start = pos;
                while (pos < end && isalpha((unsigned char)input[pos]))
                    pos++;
                if (pos > name_start && pos < end && input[pos] == ';') {
                    tv_push(v, HL_MARKUP_INLINE_ENTITY,
                            base_offset + amp, pos + 1 - amp);
                    pos++;
                    continue;
                }
            }
            /* Not a valid entity */
            pos = amp + 1;
            continue;
        }

        /* §6.1 Code span: `...` or ``...`` etc. */
        if (c == '`') {
            size_t opener_start = pos;
            size_t opener_len = 0;
            while (pos < end && input[pos] == '`') {
                opener_len++;
                pos++;
            }
            /* Must not be preceded or followed by backtick (per spec) */
            if (opener_start > 0 && input[opener_start - 1] == '`')
                goto not_code_span;
            /* Search for closing delimiter of same length */
            while (pos < end) {
                if (input[pos] == '`') {
                    size_t close_start = pos;
                    size_t close_len = 0;
                    while (pos < end && input[pos] == '`') {
                        close_len++;
                        pos++;
                    }
                    if (close_len == opener_len) {
                        /* Found matching closer */
                        tv_push(v, HL_MARKUP_INLINE_CODE,
                                base_offset + opener_start,
                                pos - opener_start);
                        goto next_inline;
                    }
                    /* Mismatched — keep scanning */
                    continue;
                }
                pos++;
            }
            /* Unclosed code span — treat opener as literal text */
            pos = opener_start + opener_len;
            continue;
        not_code_span:;
            pos = opener_start + opener_len;
            continue;
        next_inline:;
            continue;
        }

        /* §6.2 Emphasis/strong: * or _ delimiter runs */
        if (c == '*' || c == '_') {
            size_t delim_start = pos;
            size_t delim_len = 0;
            while (pos < end && input[pos] == c) {
                delim_len++;
                pos++;
            }
            /* Determine token type based on run length */
            FlareTokenType ttype;
            if (delim_len >= 2)
                ttype = HL_MARKUP_INLINE_STRONG;
            else
                ttype = HL_MARKUP_INLINE_EMPHASIS;
            tv_push(v, ttype, base_offset + delim_start, delim_len);
            continue;
        }

        /* §6.5 Autolink: <url> or <email> */
        if (c == '<') {
            size_t lt = pos;
            pos++;
            /* Try to find matching > without whitespace */
            size_t content_start = pos;
            int is_valid = 0;
            while (pos < end && input[pos] != '>' && input[pos] != ' ' &&
                   input[pos] != '\t' && input[pos] != '\n' && input[pos] != '\r') {
                pos++;
            }
            if (pos < end && input[pos] == '>' && pos > content_start) {
                /* Check it looks like a URL or email */
                size_t content_len = pos - content_start;
                const char *content = input + content_start;
                /* Simple heuristic: contains : or @ */
                for (size_t k = 0; k < content_len; k++) {
                    if (content[k] == ':' || content[k] == '@') {
                        is_valid = 1;
                        break;
                    }
                }
                if (is_valid) {
                    pos++;
                    tv_push(v, HL_MARKUP_INLINE_AUTOLINK,
                            base_offset + lt, pos - lt);
                    continue;
                }
            }
            /* Not a valid autolink — reset and maybe parse as HTML */
            pos = lt + 1;

            /* §6.6 Inline HTML tag */
            {
                size_t tag_start = lt;
                pos = lt + 1;
                int is_closing = 0;
                if (pos < end && input[pos] == '/') {
                    is_closing = 1;
                    pos++;
                }
                if (pos < end && isalpha((unsigned char)input[pos])) {
                    pos++;
                    while (pos < end && (isalnum((unsigned char)input[pos]) ||
                                         input[pos] == '-'))
                        pos++;
                    /* Skip attributes until > */
                    while (pos < end && input[pos] != '>')
                        pos++;
                    if (pos < end && input[pos] == '>') {
                        pos++;
                        tv_push(v, HL_MARKUP_INLINE_HTML,
                                base_offset + tag_start, pos - tag_start);
                        continue;
                    }
                }
                /* Not a valid HTML tag */
                pos = lt + 1;
            }
            continue;
        }

        /* §6.3 Links and §6.4 Images: ![alt](url) or [text](url)
         * Simplified: recognize the brackets and parens as link structure. */
        if (c == '!') {
            if (pos + 1 < end && input[pos + 1] == '[') {
                /* Possible image */
                size_t img_start = pos;
                pos += 2;
                /* Find matching ] */
                while (pos < end && input[pos] != ']')
                    pos++;
                if (pos < end && pos + 1 < end && input[pos + 1] == '(') {
                    pos += 2;
                    /* Find matching ) */
                    int depth = 1;
                    while (pos < end && depth > 0) {
                        if (input[pos] == '(')
                            depth++;
                        else if (input[pos] == ')')
                            depth--;
                        if (depth > 0)
                            pos++;
                    }
                    if (pos < end && depth == 0) {
                        pos++;
                        tv_push(v, HL_MARKUP_INLINE_IMAGE,
                                base_offset + img_start, pos - img_start);
                        continue;
                    }
                }
                pos = img_start + 1;
                continue;
            }
            pos++;
            continue;
        }

        if (c == '[') {
            /* Possible link */
            size_t link_start = pos;
            pos++;
            /* Find matching ] — handle nesting */
            int bracket_depth = 1;
            while (pos < end && bracket_depth > 0) {
                if (input[pos] == '[')
                    bracket_depth++;
                else if (input[pos] == ']')
                    bracket_depth--;
                if (bracket_depth > 0)
                    pos++;
            }
            if (pos < end && bracket_depth == 0) {
                /* Check for inline link: ]( */
                if (pos + 1 < end && input[pos + 1] == '(') {
                    size_t after_bracket = pos + 1;
                    pos = after_bracket + 1;
                    int depth = 1;
                    while (pos < end && depth > 0) {
                        if (input[pos] == '(')
                            depth++;
                        else if (input[pos] == ')')
                            depth--;
                        if (depth > 0)
                            pos++;
                    }
                    if (pos < end && depth == 0) {
                        pos++;
                        tv_push(v, HL_MARKUP_INLINE_LINK,
                                base_offset + link_start, pos - link_start);
                        continue;
                    }
                }
                /* Reference/collapsed/shortcut link: just the brackets */
                /* For shortcut links, the [text] itself is a link */
                pos++; /* skip ] */
                /* Check for collapsed [text][] or full [text][label] */
                if (pos < end && input[pos] == '[') {
                    pos++;
                    while (pos < end && input[pos] != ']')
                        pos++;
                    if (pos < end) {
                        pos++;
                        tv_push(v, HL_MARKUP_INLINE_LINK,
                                base_offset + link_start, pos - link_start);
                        continue;
                    }
                }
                /* Shortcut link: [text] standing alone */
                tv_push(v, HL_MARKUP_INLINE_LINK,
                        base_offset + link_start,
                        pos - link_start);
                continue;
            }
            pos = link_start + 1;
            continue;
        }

        /* §6.7 Hard line break: two+ spaces at EOL (handled by block layer) */
        /* We don't handle hard line breaks at the inline level since
         * we process line-by-line in the block phase. The block layer
         * emits HL_MARKUP_INLINE_BREAK for backslash-EOL. */

        pos++;
    }
}

/* ----- Helper: emit a line as tokens with inline parsing ---------------- */

static void emit_line_with_inlines(TokenVec *v, const char *input,
                                   size_t line_off, size_t line_len)
{
    /* First try inline tokenization; if no inlines found, emit as HL_TEXT */
    TokenVec iv = { NULL, 0, 0 };
    tokenize_inlines(&iv, input, line_off, line_off + line_len, 0);

    if (iv.count == 0) {
        tv_push_text(v, line_off, line_off + line_len);
        free(iv.items);
        return;
    }

    /* Interleave HL_TEXT gaps between inline tokens */
    size_t cur = line_off;
    for (size_t i = 0; i < iv.count; i++) {
        if (iv.items[i].offset > cur)
            tv_push_text(v, cur, iv.items[i].offset);
        tv_push(v, iv.items[i].type, iv.items[i].offset, iv.items[i].length);
        cur = iv.items[i].offset + iv.items[i].length;
    }
    if (cur < line_off + line_len)
        tv_push_text(v, cur, line_off + line_len);

    free(iv.items);
}

/* ----- Helper: sub-lex fenced code content through bloom-lisp ---------- */

static void emit_sublexed_code(TokenVec *v, const char *input,
                               size_t code_start, size_t code_len,
                               Environment *env)
{
    if (!env || code_len == 0) {
        tv_push_text(v, code_start, code_start + code_len);
        return;
    }

    FlareLexer *sub = flare_lexer_bloom_lisp(env);
    if (!sub) {
        tv_push_text(v, code_start, code_start + code_len);
        return;
    }

    size_t sub_count = 0;
    FlareToken *sub_tokens = flare_lex(sub, input + code_start, code_len, &sub_count);
    if (sub_tokens && sub_count > 0) {
        for (size_t j = 0; j < sub_count; j++) {
            tv_push(v, sub_tokens[j].type,
                    sub_tokens[j].offset + code_start,
                    sub_tokens[j].length);
        }
        free(sub_tokens);
    } else {
        tv_push_text(v, code_start, code_start + code_len);
        free(sub_tokens);
    }
    flare_lexer_free(sub);
}

/* ----- Main scan function ---------------------------------------------- */

static FlareToken *commonmark_scan(const char *input, size_t len,
                                   void *ctx, size_t *out_count)
{
    TokenVec v = { NULL, 0, 0 };
    CommonmarkCtx *mctx = ctx;
    LineIter it = { input, len, 0 };

    /* Track whether previous line was non-blank paragraph text,
     * for setext heading detection. */
    int prev_was_text = 0;
    size_t prev_text_off = 0;
    size_t prev_text_len = 0;

    while (it.pos < len) {
        size_t line_len = 0;
        size_t line_off = it.pos;
        const char *line = line_next(&it, &line_len);

        if (!line) {
            if (line_off < len)
                tv_push_text(&v, line_off, len);
            break;
        }

        line_off = line_offset(&it, line);

        /* Blank line */
        if (is_blank_line(line, line_len)) {
            tv_push_text(&v, line_off, line_off + line_len);
            size_t nl_off = line_off + line_len;
            if (nl_off < len && input[nl_off] == '\n')
                tv_push_text(&v, nl_off, nl_off + 1);
            prev_was_text = 0;
            continue;
        }

        /* --- §4.3 Setext heading (must precede thematic break check,
         *     per spec setext takes precedence when preceded by text) --- */
        {
            size_t indent_bytes = 0, underline_len = 0;
            int heading_level = 0;
            if (is_setext_underline(line, line_len, &indent_bytes,
                                    &underline_len, &heading_level)) {
                if (prev_was_text) {
                    tv_push(&v, HL_MARKUP_SETEXT_UNDERLINE, line_off, line_len);
                    size_t nl_off = line_off + line_len;
                    if (nl_off < len && input[nl_off] == '\n')
                        tv_push_text(&v, nl_off, nl_off + 1);
                    prev_was_text = 0;
                    continue;
                }
                /* Not preceded by text — falls through to thematic break or text */
            }
        }

        /* --- §4.1 Thematic break --- */
        if (is_thematic_break(line, line_len)) {
            tv_push(&v, HL_MARKUP_THEMATIC_BREAK, line_off, line_len);
            size_t nl_off = line_off + line_len;
            if (nl_off < len && input[nl_off] == '\n')
                tv_push_text(&v, nl_off, nl_off + 1);
            prev_was_text = 0;
            continue;
        }

        /* --- §4.2 ATX heading --- */
        {
            size_t indent_bytes = 0, marker_len = 0;
            if (is_atx_heading(line, line_len, &indent_bytes, &marker_len)) {
                if (indent_bytes > 0)
                    tv_push_text(&v, line_off, line_off + indent_bytes);

                tv_push(&v, HL_MARKUP_HEADING_MARKER,
                        line_off + indent_bytes, marker_len);

                size_t after_marker = indent_bytes + marker_len;
                /* Space after markers */
                if (after_marker < line_len &&
                    (line[after_marker] == ' ' || line[after_marker] == '\t')) {
                    size_t space_end = after_marker + 1;
                    while (space_end < line_len &&
                           (line[space_end] == ' ' || line[space_end] == '\t'))
                        space_end++;
                    tv_push_text(&v, line_off + after_marker, line_off + space_end);
                    after_marker = space_end;
                }

                /* Heading text — check for optional closing # sequence */
                if (after_marker < line_len) {
                    /* Look for trailing #s preceded by space(s) */
                    size_t text_end = line_len;
                    /* Trim trailing spaces */
                    while (text_end > after_marker &&
                           (line[text_end - 1] == ' ' || line[text_end - 1] == '\t'))
                        text_end--;

                    /* Check if what's left ends with #s preceded by space */
                    size_t closing_hash_start = text_end;
                    while (closing_hash_start > after_marker &&
                           line[closing_hash_start - 1] == '#')
                        closing_hash_start--;

                    if (closing_hash_start < text_end && closing_hash_start > after_marker &&
                        (line[closing_hash_start - 1] == ' ' || line[closing_hash_start - 1] == '\t')) {
                        /* Emit text before closing #s */
                        emit_line_with_inlines(&v, input,
                                               line_off + after_marker,
                                               closing_hash_start - after_marker);
                        /* Emit closing #s as marker */
                        tv_push(&v, HL_MARKUP_HEADING_MARKER,
                                line_off + closing_hash_start,
                                text_end - closing_hash_start);
                    } else {
                        /* No closing #s — all text is heading content */
                        emit_line_with_inlines(&v, input,
                                               line_off + after_marker,
                                               text_end - after_marker);
                    }
                }

                size_t nl_off = line_off + line_len;
                if (nl_off < len && input[nl_off] == '\n')
                    tv_push_text(&v, nl_off, nl_off + 1);
                prev_was_text = 0;
                continue;
            }
        }

        /* --- §4.5 Fenced code block --- */
        {
            size_t indent_bytes = 0;
            char fence_char = 0;
            size_t fence_len = 0, info_start = 0, info_len = 0;

            if (is_opening_fence(line, line_len, &indent_bytes, &fence_char,
                                 &fence_len, &info_start, &info_len)) {
                /* Leading indent */
                if (indent_bytes > 0)
                    tv_push_text(&v, line_off, line_off + indent_bytes);

                /* Opening fence marker */
                tv_push(&v, HL_MARKUP_FENCED_OPEN,
                        line_off + indent_bytes, fence_len);

                /* Spaces between fence and info string */
                if (info_start > indent_bytes + fence_len)
                    tv_push_text(&v, line_off + indent_bytes + fence_len,
                                 line_off + info_start);

                /* Info string */
                if (info_len > 0)
                    tv_push(&v, HL_MARKUP_FENCED_INFO,
                            line_off + info_start, info_len);

                /* Trailing spaces after info string */
                size_t info_end = info_start + info_len;
                if (info_end < line_len)
                    tv_push_text(&v, line_off + info_end, line_off + line_len);

                /* Newline */
                size_t nl_off = line_off + line_len;
                if (nl_off < len && input[nl_off] == '\n')
                    tv_push_text(&v, nl_off, nl_off + 1);

                int sub_lex = is_lisp_info(line, info_start, info_len);

                /* Collect code content until closing fence */
                size_t code_start = it.pos;

                while (it.pos < len) {
                    size_t inner_off = it.pos;
                    size_t inner_len = 0;
                    const char *inner_line = line_next(&it, &inner_len);
                    if (!inner_line)
                        break;

                    if (is_closing_fence(inner_line, inner_len,
                                         fence_char, fence_len)) {
                        /* Emit code content */
                        size_t code_content_len = inner_off - code_start;
                        if (code_content_len > 0 && input[inner_off - 1] == '\n')
                            code_content_len--;

                        if (sub_lex && code_content_len > 0)
                            emit_sublexed_code(&v, input, code_start,
                                               code_content_len, mctx->env);
                        else if (code_content_len > 0)
                            tv_push_text(&v, code_start,
                                         code_start + code_content_len);

                        /* Newline before closing fence */
                        if (code_content_len > 0)
                            tv_push_text(&v, code_start + code_content_len,
                                         code_start + code_content_len + 1);

                        /* Closing fence */
                        tv_push(&v, HL_MARKUP_FENCED_CLOSE,
                                line_offset(&it, inner_line), inner_len);

                        size_t close_nl = line_offset(&it, inner_line) + inner_len;
                        if (close_nl < len && input[close_nl] == '\n')
                            tv_push_text(&v, close_nl, close_nl + 1);

                        goto next_line;
                    }
                }

                /* Unclosed fence */
                if (it.pos > code_start) {
                    size_t rem = it.pos - code_start;
                    if (sub_lex)
                        emit_sublexed_code(&v, input, code_start, rem, mctx->env);
                    else
                        tv_push_text(&v, code_start, it.pos);
                }

                prev_was_text = 0;
                continue;
            }
        }

        /* --- §4.6 HTML block --- */
        if (is_html_block_start(line, line_len)) {
            tv_push(&v, HL_MARKUP_HTML_BLOCK, line_off, line_len);
            size_t nl_off = line_off + line_len;
            if (nl_off < len && input[nl_off] == '\n')
                tv_push_text(&v, nl_off, nl_off + 1);

            /* Continue until blank line (simplified — types 6 & 7)
             * or matching end tag (types 1-5). For simplicity we just
             * emit until a blank line. */
            while (it.pos < len) {
                size_t inner_off = it.pos;
                size_t inner_len = 0;
                const char *inner_line = line_next(&it, &inner_len);
                if (!inner_line)
                    break;
                if (is_blank_line(inner_line, inner_len)) {
                    /* Don't consume the blank line */
                    it.pos = inner_off;
                    break;
                }
                tv_push(&v, HL_MARKUP_HTML_BLOCK, inner_off, inner_len);
                size_t inner_nl = inner_off + inner_len;
                if (inner_nl < len && input[inner_nl] == '\n')
                    tv_push_text(&v, inner_nl, inner_nl + 1);
            }
            prev_was_text = 0;
            continue;
        }

        /* --- §4.4 Indented code block --- */
        {
            size_t indent_bytes = 0;
            if (is_indented_code(line, line_len, &indent_bytes)) {
                /* Indent (4 spaces) */
                tv_push_text(&v, line_off, line_off + 4);
                /* Code content (the rest minus indent) */
                tv_push(&v, HL_MARKUP_INDENTED_CODE,
                        line_off + 4, line_len - 4);
                size_t nl_off = line_off + line_len;
                if (nl_off < len && input[nl_off] == '\n')
                    tv_push_text(&v, nl_off, nl_off + 1);

                /* Continue while indented or blank */
                while (it.pos < len) {
                    size_t inner_off = it.pos;
                    size_t inner_len = 0;
                    const char *inner_line = line_next(&it, &inner_len);
                    if (!inner_line)
                        break;
                    if (is_blank_line(inner_line, inner_len)) {
                        tv_push_text(&v, inner_off, inner_off + inner_len);
                        size_t inner_nl = inner_off + inner_len;
                        if (inner_nl < len && input[inner_nl] == '\n')
                            tv_push_text(&v, inner_nl, inner_nl + 1);
                        continue;
                    }
                    size_t ib = 0;
                    if (!is_indented_code(inner_line, inner_len, &ib)) {
                        it.pos = inner_off;
                        break;
                    }
                    tv_push_text(&v, inner_off, inner_off + 4);
                    tv_push(&v, HL_MARKUP_INDENTED_CODE,
                            inner_off + 4, inner_len - 4);
                    size_t inner_nl = inner_off + inner_len;
                    if (inner_nl < len && input[inner_nl] == '\n')
                        tv_push_text(&v, inner_nl, inner_nl + 1);
                }
                prev_was_text = 0;
                continue;
            }
        }

        /* --- §5.1 Block quote --- */
        {
            size_t indent_bytes = 0, marker_len = 0;
            if (is_block_quote(line, line_len, &indent_bytes, &marker_len)) {
                if (indent_bytes > 0)
                    tv_push_text(&v, line_off, line_off + indent_bytes);
                tv_push(&v, HL_MARKUP_BLOCKQUOTE_MARKER,
                        line_off + indent_bytes, marker_len);
                /* Rest of line as text with inlines */
                size_t rest_start = indent_bytes + marker_len;
                if (rest_start < line_len)
                    emit_line_with_inlines(&v, input,
                                           line_off + rest_start,
                                           line_len - rest_start);
                size_t nl_off = line_off + line_len;
                if (nl_off < len && input[nl_off] == '\n')
                    tv_push_text(&v, nl_off, nl_off + 1);

                /* Lazy continuation: subsequent lines without > that
                 * could be paragraph continuation. We just emit them
                 * as regular lines (the block quote context is visual). */
                prev_was_text = 0;
                continue;
            }
        }

        /* --- §5.2 List item --- */
        {
            size_t indent_bytes = 0, marker_len = 0;
            if (is_list_item(line, line_len, &indent_bytes, &marker_len)) {
                if (indent_bytes > 0)
                    tv_push_text(&v, line_off, line_off + indent_bytes);
                tv_push(&v, HL_MARKUP_LIST_MARKER,
                        line_off + indent_bytes, marker_len);
                /* Rest of line as text with inlines */
                size_t rest_start = indent_bytes + marker_len;
                if (rest_start < line_len)
                    emit_line_with_inlines(&v, input,
                                           line_off + rest_start,
                                           line_len - rest_start);
                size_t nl_off = line_off + line_len;
                if (nl_off < len && input[nl_off] == '\n')
                    tv_push_text(&v, nl_off, nl_off + 1);
                prev_was_text = 1;
                prev_text_off = line_off;
                prev_text_len = line_len;
                continue;
            }
        }

        /* --- §4.7 Link reference definition --- */
        {
            size_t indent_bytes = 0;
            if (is_link_ref_def(line, line_len, &indent_bytes)) {
                if (indent_bytes > 0)
                    tv_push_text(&v, line_off, line_off + indent_bytes);
                tv_push(&v, HL_MARKUP_LINKREF_DEF,
                        line_off + indent_bytes, line_len - indent_bytes);
                size_t nl_off = line_off + line_len;
                if (nl_off < len && input[nl_off] == '\n')
                    tv_push_text(&v, nl_off, nl_off + 1);
                prev_was_text = 0;
                continue;
            }
        }

        /* --- §4.8 Paragraph (default) --- */
        {
            /* Check for hard line break at end: two+ trailing spaces or
             * backslash before newline */
            size_t text_end = line_len;
            int hard_break = 0;
            if (text_end > 0) {
                /* Check for trailing backslash */
                if (line[text_end - 1] == '\\') {
                    hard_break = 1;
                    text_end--;
                } else {
                    /* Check for two+ trailing spaces */
                    size_t trailing = 0;
                    while (trailing < text_end &&
                           (line[text_end - 1 - trailing] == ' ' ||
                            line[text_end - 1 - trailing] == '\t'))
                        trailing++;
                    if (trailing >= 2) {
                        hard_break = 1;
                        text_end = line_len - trailing;
                    }
                }
            }

            emit_line_with_inlines(&v, input, line_off, text_end);

            if (hard_break) {
                tv_push(&v, HL_MARKUP_INLINE_BREAK,
                        line_off + text_end, line_len - text_end);
            }

            size_t nl_off = line_off + line_len;
            if (nl_off < len && input[nl_off] == '\n')
                tv_push_text(&v, nl_off, nl_off + 1);

            prev_was_text = 1;
            prev_text_off = line_off;
            prev_text_len = line_len;
            continue;
        }

    next_line:
        prev_was_text = 0;
        continue;
    }

    *out_count = v.count;
    return v.items;
}

/* ----- Context lifecycle ------------------------------------------------ */

static void commonmark_ctx_free(void *p)
{
    free(p);
}

/* ----- Public constructor ----------------------------------------------- */

FlareLexer *flare_lexer_commonmark(Environment *env)
{
    if (!env)
        return NULL;

    CommonmarkCtx *ctx = malloc(sizeof(CommonmarkCtx));
    if (!ctx)
        return NULL;
    ctx->env = env;

    return flare_lexer_alloc("commonmark", ctx, commonmark_scan,
                             commonmark_ctx_free);
}

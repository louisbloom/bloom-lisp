/* lexer_bloom_lisp.c - Bloom Lisp scanner
 *
 * Classification uses the bloom-lisp runtime as the single source of truth:
 *   - Special forms: lisp_sf_kind() from bloom-lisp (auto-generated from
 *     the SPECIAL_FORMS X-macro, maps each form to its semantic kind).
 *   - Builtins/functions: resolved via env_lookup on the Environment.
 *
 * An Environment* is required. Pass the result of lisp_init().
 */

#include "../include/bloom-lisp/highlight.h"
#include "../include/lisp.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration from lexer.c */
extern BflareLexer *bflare_lexer_alloc(const char *name, void *ctx,
                                       BflareToken *(*scan)(const char *, size_t, void *, size_t *),
                                       void (*free_ctx)(void *));

typedef struct
{
    Environment *env;
} BloomLispCtx;

/* Map bloom-lisp's SfKind to bloom-flare's token subcategory */
static BflareTokenType sf_kind_to_type(SfKind kind)
{
    switch (kind) {
    case SF_KIND_DEFINE:
        return HL_KEYWORD_DEFINE;
    case SF_KIND_CONTROL:
        return HL_KEYWORD_CONTROL;
    case SF_KIND_MACRO_DEF:
        return HL_KEYWORD_MACRO_DEF;
    default:
        return HL_KEYWORD_SPECIAL_FORM;
    }
}

/* ----- Atom classification ----- */

static int is_delimiter(char c)
{
    return c == '\0' || isspace((unsigned char)c) ||
           c == '(' || c == ')' || c == ';' ||
           c == '"' || c == '\'' || c == '`' || c == ',';
}

typedef struct
{
    BflareToken *items;
    size_t count;
    size_t capacity;
} TokenVec;

static void tv_push(TokenVec *v, BflareTokenType type, size_t offset, size_t length)
{
    if (v->count >= v->capacity) {
        v->capacity = v->capacity ? v->capacity * 2 : 16;
        v->items = realloc(v->items, v->capacity * sizeof(BflareToken));
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

static BflareTokenType classify_atom(const char *input, size_t offset, size_t length,
                                     const BloomLispCtx *ctx)
{
    const char *s = input + offset;

    if (s[0] == ':' && length > 1)
        return HL_NAME_KEYWORD_ARG;

    if (length == 3 && strncmp(s, "nil", 3) == 0)
        return HL_LITERAL_NIL;
    if (length == 2 && strncmp(s, "#f", 2) == 0)
        return HL_LITERAL_NIL;
    if (length == 2 && strncmp(s, "#t", 2) == 0)
        return HL_LITERAL_BOOLEAN;

    /* Number check */
    {
        const char *p = s;
        int is_num = 1, has_dot = 0;
        size_t remain = length;

        if (*p == '-' || *p == '+') {
            p++;
            remain--;
        }
        if (remain == 0)
            is_num = 0;
        while (remain > 0 && is_num) {
            if (*p == '.') {
                if (has_dot) {
                    is_num = 0;
                    break;
                }
                has_dot = 1;
            } else if (!isdigit((unsigned char)*p)) {
                is_num = 0;
            }
            p++;
            remain--;
        }
        if (is_num && length > 0)
            return HL_LITERAL_NUMBER;
    }

    /* Symbol classification via runtime */
    char buf[256];
    if (length >= sizeof(buf))
        return HL_NAME_VARIABLE;

    memcpy(buf, s, length);
    buf[length] = '\0';

    LispObject *sym = lisp_intern(buf);

    /* Special forms: ask bloom-lisp.
     * Returns -1 for non-special-forms; must cast to int because
     * SfKind is an enum and GCC may treat the comparison as unsigned. */
    int kind = (int)lisp_sf_kind(sym);
    if (kind >= 0)
        return sf_kind_to_type(kind);

    /* Environment lookup: builtin, function, macro, or variable */
    LispObject *val = env_lookup(ctx->env, LISP_SYM_VAL(sym));
    if (val) {
        LispType t = LISP_TYPE(val);
        if (t == LISP_BUILTIN)
            return HL_NAME_BUILTIN;
        if (t == LISP_LAMBDA || t == LISP_MACRO)
            return HL_NAME_FUNCTION;
    }

    return HL_NAME_VARIABLE;
}

/* Main scan function */
static BflareToken *bloom_lisp_scan(const char *input, size_t len,
                                    void *ctx, size_t *out_count)
{
    TokenVec v = { NULL, 0, 0 };
    size_t pos = 0;
    BloomLispCtx *lctx = ctx;

    while (pos < len) {
        char c = input[pos];

        if (isspace((unsigned char)c)) {
            size_t start = pos;
            while (pos < len && isspace((unsigned char)input[pos]))
                pos++;
            tv_push_text(&v, start, pos);
            continue;
        }

        if (c == ';') {
            size_t start = pos;
            while (pos < len && input[pos] != '\n')
                pos++;
            tv_push(&v, HL_COMMENT_LINE, start, pos - start);
            continue;
        }

        if (c == '"') {
            size_t start = pos;
            pos++;
            while (pos < len && input[pos] != '"') {
                if (input[pos] == '\\' && pos + 1 < len)
                    pos++;
                pos++;
            }
            if (pos < len) {
                pos++;
                tv_push(&v, HL_LITERAL_STRING, start, pos - start);
            } else {
                tv_push(&v, HL_ERROR_UNCLOSED_STRING, start, pos - start);
            }
            continue;
        }

        if (c == '#') {
            if (pos + 1 < len) {
                char next = input[pos + 1];
                if (next == '\\') {
                    size_t start = pos;
                    pos += 2;
                    if (pos < len) {
                        if (isalpha((unsigned char)input[pos])) {
                            while (pos < len && !is_delimiter(input[pos]) &&
                                   input[pos] != ')' && input[pos] != '(')
                                pos++;
                        } else {
                            if (pos < len)
                                pos++;
                        }
                    }
                    tv_push(&v, HL_LITERAL_CHARACTER, start, pos - start);
                    continue;
                }
                if (next == '(') {
                    tv_push(&v, HL_PUNCT_HASH, pos, 1);
                    pos++;
                    tv_push(&v, HL_PUNCT_OPEN_PAREN, pos, 1);
                    pos++;
                    continue;
                }
                if (next == 't') {
                    tv_push(&v, HL_LITERAL_BOOLEAN, pos, 2);
                    pos += 2;
                    continue;
                }
                if (next == 'f') {
                    tv_push(&v, HL_LITERAL_NIL, pos, 2);
                    pos += 2;
                    continue;
                }
            }
            tv_push(&v, HL_PUNCT_HASH, pos, 1);
            pos++;
            continue;
        }

        if (c == '(') {
            tv_push(&v, HL_PUNCT_OPEN_PAREN, pos, 1);
            pos++;
            continue;
        }
        if (c == ')') {
            tv_push(&v, HL_PUNCT_CLOSE_PAREN, pos, 1);
            pos++;
            continue;
        }

        if (c == '\'') {
            tv_push(&v, HL_OPERATOR_QUOTE, pos, 1);
            pos++;
            continue;
        }
        if (c == '`') {
            tv_push(&v, HL_OPERATOR_BACKQUOTE, pos, 1);
            pos++;
            continue;
        }
        if (c == ',') {
            if (pos + 1 < len && input[pos + 1] == '@') {
                tv_push(&v, HL_OPERATOR_UNQUOTE_SPLICING, pos, 2);
                pos += 2;
            } else {
                tv_push(&v, HL_OPERATOR_UNQUOTE, pos, 1);
                pos++;
            }
            continue;
        }

        if (c == '.' && pos + 1 < len &&
            (isspace((unsigned char)input[pos + 1]) || input[pos + 1] == ')')) {
            tv_push(&v, HL_PUNCT_DOT, pos, 1);
            pos++;
            continue;
        }

        /* Atom */
        {
            size_t start = pos;
            while (pos < len && !is_delimiter(input[pos]) &&
                   !(input[pos] == '.' && pos + 1 < len &&
                     (isspace((unsigned char)input[pos + 1]) || input[pos + 1] == ')')))
                pos++;

            if (pos > start) {
                BflareTokenType t = classify_atom(input, start, pos - start, lctx);
                tv_push(&v, t, start, pos - start);
            } else {
                tv_push(&v, HL_TEXT, pos, 1);
                pos++;
            }
        }
    }

    *out_count = v.count;
    return v.items;
}

static void lctx_free(void *p)
{
    free(p);
}

BflareLexer *bflare_lexer_bloom_lisp(Environment *env)
{
    if (!env)
        return NULL;

    BloomLispCtx *ctx = calloc(1, sizeof(BloomLispCtx));
    if (!ctx)
        return NULL;

    ctx->env = env;

    return bflare_lexer_alloc("bloom-lisp", ctx,
                              bloom_lisp_scan,
                              lctx_free);
}

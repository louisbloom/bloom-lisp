/* lexer.c - BflareLexer interface + registry */

#include "../include/bloom-lisp/highlight.h"
#include <stdlib.h>
#include <string.h>

struct BflareLexer
{
    char *name;
    BflareToken *(*scan)(const char *input, size_t len, void *ctx, size_t *count);
    void *ctx;
    void (*free_ctx)(void *ctx);
};

BflareLexer *bflare_lexer_get(const char *name)
{
    if (strcmp(name, "bloom-lisp") == 0)
        return bflare_lexer_bloom_lisp(NULL);
    return NULL;
}

void bflare_lexer_free(BflareLexer *lexer)
{
    if (!lexer)
        return;
    if (lexer->free_ctx)
        lexer->free_ctx(lexer->ctx);
    free(lexer->name);
    free(lexer);
}

BflareToken *bflare_lex(const BflareLexer *lexer, const char *input,
                        size_t input_len, size_t *out_count)
{
    if (!lexer || !input || !out_count) {
        if (out_count)
            *out_count = 0;
        return NULL;
    }
    return lexer->scan(input, input_len, lexer->ctx, out_count);
}

/* Internal: create a lexer struct (used by language-specific constructors) */
BflareLexer *bflare_lexer_alloc(const char *name, void *ctx,
                                BflareToken *(*scan)(const char *, size_t, void *, size_t *),
                                void (*free_ctx)(void *))
{
    BflareLexer *l = malloc(sizeof(BflareLexer));
    if (!l)
        return NULL;
    l->name = strdup(name);
    l->scan = scan;
    l->ctx = ctx;
    l->free_ctx = free_ctx;
    return l;
}

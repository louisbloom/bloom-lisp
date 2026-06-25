/* lexer.c - FlareLexer interface + registry */

#include "../include/bloom-lisp/highlight.h"
#include <stdlib.h>
#include <string.h>

struct FlareLexer
{
    char *name;
    FlareToken *(*scan)(const char *input, size_t len, void *ctx, size_t *count);
    void *ctx;
    void (*free_ctx)(void *ctx);
};

FlareLexer *flare_lexer_get(const char *name)
{
    if (strcmp(name, "bloom-lisp") == 0)
        return flare_lexer_bloom_lisp(NULL);
    if (strcmp(name, "commonmark") == 0 || strcmp(name, "markdown") == 0)
        return flare_lexer_commonmark(NULL);
    return NULL;
}

void flare_lexer_free(FlareLexer *lexer)
{
    if (!lexer)
        return;
    if (lexer->free_ctx)
        lexer->free_ctx(lexer->ctx);
    free(lexer->name);
    free(lexer);
}

FlareToken *flare_lex(const FlareLexer *lexer, const char *input,
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
FlareLexer *flare_lexer_alloc(const char *name, void *ctx,
                              FlareToken *(*scan)(const char *, size_t, void *, size_t *),
                              void (*free_ctx)(void *))
{
    FlareLexer *l = malloc(sizeof(FlareLexer));
    if (!l)
        return NULL;
    l->name = strdup(name);
    l->scan = scan;
    l->ctx = ctx;
    l->free_ctx = free_ctx;
    return l;
}

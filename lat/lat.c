/* lat.c - "lisp cat": highlight source files to the terminal
 *
 * Usage: lat [OPTIONS] [FILE...]
 *
 * Reads each FILE (or stdin if no files / '-') and writes
 * syntax-highlighted ANSI output to stdout.
 *
 * Options:
 *   -f, --format FORMAT   output color format: truecolor (default), 256, 16, 8
 *   -s, --style STYLE     color style: dracula (default), monokai, github-dark, github-light
 *   -l, --language LANG   lexer language: bloom-lisp, commonmark/markdown, auto (default)
 *   -h, --help            show this help text
 *   -v, --version         show version
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "../include/bloom-lisp/highlight.h"
#include "../include/lisp.h"
#include "bloom_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGNAME "lat"

static const char *version_string(void)
{
#ifdef BLOOM_LISP_VERSION
    return BLOOM_LISP_VERSION;
#else
    return "unknown";
#endif
}

typedef enum
{
    LANG_AUTO,
    LANG_BLOOM_LISP,
    LANG_COMMONMARK
} LangChoice;

static void usage(void)
{
    printf(
        "Usage: %s [OPTIONS] [FILE...]\n"
        "\n"
        "Highlight source files with ANSI terminal colors.\n"
        "Reads from FILE, or stdin if no files given (use '-' for explicit stdin).\n"
        "\n"
        "Options:\n"
        "  -f, --format FORMAT   output color format: truecolor (default), 256, 16, 8\n"
        "  -s, --style STYLE     color style: dracula (default), monokai,\n"
        "                        github-dark, github-light\n"
        "  -l, --language LANG   lexer language: auto (default), bloom-lisp,\n"
        "                        commonmark, markdown\n"
        "  -h, --help            show this help text\n"
        "  -v, --version         show version\n"
        "\n"
        "Formats:\n"
        "  truecolor    24-bit RGB (SGR 38;2;R;G;B) — most terminals\n"
        "  256          xterm-256 palette (SGR 38;5;N)\n"
        "  16           ANSI 16-color (aixterm bright: SGR 30-37, 90-97)\n"
        "  8            ANSI 8-color (SGR 30-37, bold for bright)\n"
        "\n"
        "Styles:\n"
        "  dracula       dark background, purples and pinks\n"
        "  monokai       dark background, vivid pastels\n"
        "  github-dark   GitHub dark theme\n"
        "  github-light  GitHub light theme\n"
        "\n"
        "Languages:\n"
        "  auto          detect from file extension (default)\n"
        "  bloom-lisp    Bloom Lisp source\n"
        "  commonmark    CommonMark/Markdown (highlights fenced lisp code blocks)\n"
        "  markdown      alias for commonmark\n",
        PROGNAME);
}

static int parse_format(const char *s, FlareColorDepth *out)
{
    if (strcmp(s, "truecolor") == 0) {
        *out = BFLARE_COLOR_TRUECOLOR;
        return 0;
    }
    if (strcmp(s, "256") == 0) {
        *out = BFLARE_COLOR_256;
        return 0;
    }
    if (strcmp(s, "16") == 0) {
        *out = BFLARE_COLOR_16;
        return 0;
    }
    if (strcmp(s, "8") == 0) {
        *out = BFLARE_COLOR_8;
        return 0;
    }
    fprintf(stderr, "%s: unknown format '%s' (choose: truecolor, 256, 16, 8)\n",
            PROGNAME, s);
    return -1;
}

typedef FlareStyle *(*style_ctor)(void);

static int parse_style(const char *s, style_ctor *out)
{
    if (strcmp(s, "dracula") == 0) {
        *out = flare_style_dracula;
        return 0;
    }
    if (strcmp(s, "monokai") == 0) {
        *out = flare_style_monokai;
        return 0;
    }
    if (strcmp(s, "github-dark") == 0) {
        *out = flare_style_github_dark;
        return 0;
    }
    if (strcmp(s, "github-light") == 0) {
        *out = flare_style_github_light;
        return 0;
    }
    fprintf(stderr,
            "%s: unknown style '%s' (choose: dracula, monokai, github-dark, github-light)\n",
            PROGNAME, s);
    return -1;
}

static int parse_language(const char *s, LangChoice *out)
{
    if (strcmp(s, "auto") == 0) {
        *out = LANG_AUTO;
        return 0;
    }
    if (strcmp(s, "bloom-lisp") == 0) {
        *out = LANG_BLOOM_LISP;
        return 0;
    }
    if (strcmp(s, "commonmark") == 0 || strcmp(s, "markdown") == 0) {
        *out = LANG_COMMONMARK;
        return 0;
    }
    fprintf(stderr,
            "%s: unknown language '%s' (choose: auto, bloom-lisp, commonmark, markdown)\n",
            PROGNAME, s);
    return -1;
}

/* Detect language from file extension */
static LangChoice detect_language(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (!dot)
        return LANG_BLOOM_LISP;

    if (strcmp(dot, ".md") == 0 || strcmp(dot, ".markdown") == 0)
        return LANG_COMMONMARK;

    return LANG_BLOOM_LISP;
}

static char *read_all(FILE *f, size_t *out_len)
{
    size_t cap = 4096;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf)
        return NULL;

    size_t n;
    while ((n = fread(buf + len, 1, cap - len, f)) > 0) {
        len += n;
        if (len == cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) {
                free(buf);
                return NULL;
            }
            buf = tmp;
        }
    }
    buf[len] = '\0';
    *out_len = len;
    return buf;
}

static int highlight_file(const char *path, Environment *env,
                          LangChoice lang, FlareStyle *style,
                          FlareFormatter *formatter)
{
    FILE *f;
    int close_it = 0;

    if (strcmp(path, "-") == 0) {
        f = stdin;
    } else {
        f = fopen(path, "rb");
        if (!f) {
            fprintf(stderr, "%s: %s: ", PROGNAME, path);
            perror(NULL);
            return 1;
        }
        close_it = 1;
    }

    size_t src_len = 0;
    char *src = read_all(f, &src_len);
    if (close_it)
        fclose(f);

    if (!src) {
        fprintf(stderr, "%s: out of memory reading %s\n", PROGNAME, path);
        return 1;
    }

    LangChoice effective = lang;
    if (effective == LANG_AUTO)
        effective = detect_language(path);

    FlareLexer *lexer;
    if (effective == LANG_COMMONMARK)
        lexer = flare_lexer_commonmark(env);
    else
        lexer = flare_lexer_bloom_lisp(env);

    if (!lexer) {
        fprintf(stderr, "%s: failed to create lexer for %s\n", PROGNAME, path);
        free(src);
        return 1;
    }

    FlareResult r = flare_highlight(src, src_len, lexer, style, formatter);
    flare_lexer_free(lexer);
    free(src);

    if (!r.data) {
        fprintf(stderr, "%s: highlight failed for %s\n", PROGNAME, path);
        return 1;
    }

    fwrite(r.data, 1, r.length, stdout);
    flare_result_free(r);
    return 0;
}

int main(int argc, char **argv)
{
    FlareColorDepth depth = BFLARE_COLOR_TRUECOLOR;
    style_ctor make_style = flare_style_dracula;
    LangChoice lang = LANG_AUTO;
    int file_start = argc;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("%s %s\n", PROGNAME, version_string());
            return 0;
        }
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: %s requires an argument\n", PROGNAME, argv[i]);
                return 2;
            }
            i++;
            if (parse_format(argv[i], &depth) != 0)
                return 2;
            continue;
        }
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--style") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: %s requires an argument\n", PROGNAME, argv[i]);
                return 2;
            }
            i++;
            if (parse_style(argv[i], &make_style) != 0)
                return 2;
            continue;
        }
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--language") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: %s requires an argument\n", PROGNAME, argv[i]);
                return 2;
            }
            i++;
            if (parse_language(argv[i], &lang) != 0)
                return 2;
            continue;
        }
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "%s: unknown option '%s'\n", PROGNAME, argv[i]);
            fprintf(stderr, "Try '%s --help' for more information.\n", PROGNAME);
            return 2;
        }
        file_start = i;
        break;
    }

    Environment *env = lisp_init();

    FlareStyle *style = make_style();
    FlareFormatter *fmt = flare_formatter_terminal(depth);

    int rc = 0;

    if (file_start >= argc) {
        rc = highlight_file("-", env, lang, style, fmt);
    } else {
        for (int i = file_start; i < argc; i++) {
            if (highlight_file(argv[i], env, lang, style, fmt) != 0)
                rc = 1;
        }
    }

    flare_style_free(style);
    flare_formatter_free(fmt);
    lisp_cleanup();

    return rc;
}

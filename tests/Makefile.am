# Test suite using Automake's parallel test harness
# C unit tests (compiled, run as plain executables)
FLARE_TESTS = \
    test_token_type \
    test_color \
    test_style \
    test_lexer \
    test_lexer_commonmark \
    test_formatter \
    test_api \
    test_roundtrip

check_PROGRAMS = test_sf_kind $(FLARE_TESTS)

test_sf_kind_SOURCES = test_sf_kind.c
test_sf_kind_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)
test_sf_kind_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

# Flare unit tests
AM_FLARE_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

AM_FLARE_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

test_token_type_SOURCES = test_token_type.c
test_token_type_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_token_type_LDADD = $(AM_FLARE_LDADD)

test_color_SOURCES = test_color.c
test_color_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_color_LDADD = $(AM_FLARE_LDADD)

test_style_SOURCES = test_style.c
test_style_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_style_LDADD = $(AM_FLARE_LDADD)

test_lexer_SOURCES = test_lexer.c
test_lexer_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_lexer_LDADD = $(AM_FLARE_LDADD)

test_lexer_commonmark_SOURCES = test_lexer_commonmark.c
test_lexer_commonmark_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_lexer_commonmark_LDADD = $(AM_FLARE_LDADD)

test_formatter_SOURCES = test_formatter.c
test_formatter_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_formatter_LDADD = $(AM_FLARE_LDADD)

test_api_SOURCES = test_api.c
test_api_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_api_LDADD = $(AM_FLARE_LDADD)

test_roundtrip_SOURCES = test_roundtrip.c
test_roundtrip_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_roundtrip_LDADD = $(AM_FLARE_LDADD)

# Force ditty_version.h to exist before test objects compile
$(test_sf_kind_OBJECTS): $(top_builddir)/src/ditty_version.h
$(FLARE_TESTS_OBJECTS): $(top_builddir)/src/ditty_version.h

# Lisp script tests
TESTS = \
    basic/alists.lisp \
    basic/apply.lisp \
    basic/and_or_list.lisp \
    basic/characters.lisp \
    basic/filesystem.lisp \
    basic/format.lisp \
    basic/hash_tables.lisp \
    basic/integer_ops.lisp \
    basic/integers.lisp \
    basic/keywords.lisp \
    basic/length.lisp \
    basic/list_reverse.lisp \
    basic/path_expansion.lisp \
    basic/printing.lisp \
    basic/recursion.lisp \
    basic/string-index.lisp \
    basic/string_number_conversion.lisp \
    basic/string_ports.lisp \
    basic/strings.lisp \
    basic/symbols.lisp \
    basic/packages.lisp \
    basic/vectors.lisp \
    advanced/collection_iteration.lisp \
    advanced/cond_case.lisp \
    advanced/condition-system.lisp \
    advanced/do_loop.lisp \
    advanced/docstrings.lisp \
    advanced/equality.lisp \
    advanced/file_io.lisp \
    advanced/let_star.lisp \
    advanced/lisp-fmt.lisp \
    advanced/load.lisp \
    advanced/macros.lisp \
    advanced/named_functions.lisp \
    advanced/optional-parameters.lisp \
    advanced/progn.lisp \
    advanced/quasiquote.lisp \
    advanced/regex.lisp \
    advanced/introspection.lisp \
    advanced/tail_recursion.lisp \
    advanced/utf8.lisp \
    regression/cond_case_multi_expr.lisp \
    regression/list_ref_type_check.lisp \
    regression/multiline_parsing.lisp \
    regression/nested-unknown-hash-syntax.lisp \
    regression/tail_call_unwrap.lisp \
    test_sf_kind \
    $(FLARE_TESTS)

TEST_EXTENSIONS = .lisp
LISP_LOG_COMPILER = $(srcdir)/run-test.sh

AM_TESTS_ENVIRONMENT = \
    DITTY_BIN="$(abs_top_builddir)/cli/ditty"; \
    export DITTY_BIN;

EXTRA_DIST = \
    run-test.sh \
    test_sf_kind.c \
    flare_testkit.h \
    test-helpers.lisp \
    snapshots/simple_expression_truecolor.txt \
    snapshots/simple_expression_256.txt \
    snapshots/simple_expression_16.txt \
    snapshots/define_lambda_truecolor.txt \
    snapshots/quasiquote_truecolor.txt \
    snapshots/comment_and_string_truecolor.txt \
    $(TESTS)

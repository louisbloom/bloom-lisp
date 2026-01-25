# Test suite using Automake's parallel test harness
TESTS = \
    basic/alists.lisp \
    basic/and_or_list.lisp \
    basic/characters.lisp \
    basic/format.lisp \
    basic/hash_tables.lisp \
    basic/integer_ops.lisp \
    basic/integers.lisp \
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
    advanced/tail_recursion.lisp \
    advanced/utf8.lisp \
    regression/cond_case_multi_expr.lisp \
    regression/list_ref_type_check.lisp \
    regression/multiline_parsing.lisp \
    regression/nested-unknown-hash-syntax.lisp \
    regression/tail_call_unwrap.lisp

TEST_EXTENSIONS = .lisp
LISP_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(srcdir)/run-test.sh
LISP_LOG_COMPILER = $(top_builddir)/repl/bloom-repl

AM_TESTS_ENVIRONMENT = \
    BLOOM_REPL="$(top_builddir)/repl/bloom-repl"; \
    export BLOOM_REPL;

EXTRA_DIST = \
    run-test.sh \
    test-helpers.lisp \
    $(TESTS)

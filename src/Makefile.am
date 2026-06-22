# Build static libraries
lib_LIBRARIES = libbloomlisp.a

# Generated headers: docstrings (from markdown) and version (from git state).
# bloom_version.h is regenerated on every `make` so the embedded version
# string tracks the current git HEAD (including a -dirty suffix when the
# working tree has uncommitted changes). The recipe always runs, but the
# file is only rewritten when the version actually changes — so dependent
# .o files stay cached when nothing changed.
BUILT_SOURCES = docstrings.gen.h bloom_version.h
CLEANFILES = docstrings.gen.h bloom_version.h

.PHONY: bloom_version_force
bloom_version.h: bloom_version_force
	@v=`"$(top_srcdir)/build-aux/git-version.sh" "$(top_srcdir)" 2>/dev/null`; \
	test -n "$$v" || v="$(BLOOM_LISP_VERSION)"; \
	test -n "$$v" || v="unknown"; \
	printf '#ifndef BLOOM_VERSION_H\n#define BLOOM_VERSION_H\n#define BLOOM_LISP_VERSION "%s"\n#endif\n' "$$v" > $@.tmp; \
	if cmp -s $@.tmp $@ 2>/dev/null; then \
	  rm -f $@.tmp; \
	else \
	  mv $@.tmp $@; \
	  echo "  GEN      $@ ($$v)"; \
	fi
bloom_version_force:

DOC_FILES = \
    $(top_srcdir)/doc/arithmetic.md \
    $(top_srcdir)/doc/characters.md \
    $(top_srcdir)/doc/comparison.md \
    $(top_srcdir)/doc/errors.md \
    $(top_srcdir)/doc/file-io.md \
    $(top_srcdir)/doc/filesystem.md \
    $(top_srcdir)/doc/functions.md \
    $(top_srcdir)/doc/hash-tables.md \
    $(top_srcdir)/doc/lists.md \
    $(top_srcdir)/doc/packages.md \
    $(top_srcdir)/doc/printing.md \
    $(top_srcdir)/doc/regex.md \
    $(top_srcdir)/doc/string-ports.md \
    $(top_srcdir)/doc/strings.md \
    $(top_srcdir)/doc/symbols.md \
    $(top_srcdir)/doc/time-and-profiling.md \
    $(top_srcdir)/doc/type-predicates.md \
    $(top_srcdir)/doc/vectors.md

docstrings.gen.h: $(top_srcdir)/scripts/gen-docstrings.sh $(DOC_FILES)
	$(top_srcdir)/scripts/gen-docstrings.sh $(top_srcdir)/doc > $@

# Main lisp library
libbloomlisp_a_SOURCES = \
    lisp.c \
    reader.c \
    eval.c \
    print.c \
    env.c \
    builtins.c \
    builtins_arithmetic.c \
    builtins_comparison.c \
    builtins_strings.c \
    builtins_characters.c \
    builtins_lists.c \
    builtins_vectors.c \
    builtins_hash_tables.c \
    builtins_regex.c \
    builtins_file_io.c \
    builtins_string_ports.c \
    builtins_type_predicates.c \
    builtins_symbols.c \
    builtins_functions.c \
    builtins_printing.c \
    builtins_errors.c \
    builtins_packages.c \
    builtins_filesystem.c \
    builtins_time_profiling.c \
    utf8.c \
    regex.c \
    hash_table.c \
    file_utils.c \
    flare_lexer.c \
    flare_lexer_bloom_lisp.c \
    flare_token_type.c \
    flare_style.c \
    flare_style_monokai.c \
    flare_style_dracula.c \
    flare_style_github_dark.c \
    flare_style_github_light.c \
    flare_formatter.c \
    flare_formatter_terminal.c \
    flare_color.c \
    flare_highlight.c

libbloomlisp_a_CPPFLAGS = \
    -I$(top_srcdir)/include \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

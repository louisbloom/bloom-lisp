# Build static libraries
lib_LIBRARIES = libditty.a

# Generated headers: docstrings (from markdown) and version (from git state).
# ditty_version.h is regenerated on every `make` so the embedded version
# string tracks the current git HEAD (including a -dirty suffix when the
# working tree has uncommitted changes). The recipe always runs, but the
# file is only rewritten when the version actually changes — so dependent
# .o files stay cached when nothing changed.
BUILT_SOURCES = docstrings.gen.h ditty_version.h
CLEANFILES = docstrings.gen.h ditty_version.h

.PHONY: ditty_version_force
ditty_version.h: ditty_version_force
	@v=`"$(top_srcdir)/build-aux/git-version.sh" "$(top_srcdir)" 2>/dev/null`; \
	test -n "$$v" || v="$(DITTY_VERSION)"; \
	test -n "$$v" || v="unknown"; \
	printf '#ifndef DITTY_VERSION_H\n#define DITTY_VERSION_H\n#define DITTY_VERSION "%s"\n#endif\n' "$$v" > $@.tmp; \
	if cmp -s $@.tmp $@ 2>/dev/null; then \
	  rm -f $@.tmp; \
	else \
	  mv $@.tmp $@; \
	  echo "  GEN      $@ ($$v)"; \
	fi
ditty_version_force:

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
libditty_a_SOURCES = \
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
    flare_lexer_ditty.c \
    flare_lexer_commonmark.c \
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

libditty_a_CPPFLAGS = \
    -I$(top_srcdir)/include \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

# Build static libraries
lib_LIBRARIES = libbloomlisp.a

# Generate docstrings header from markdown files
BUILT_SOURCES = docstrings.gen.h
CLEANFILES = docstrings.gen.h

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
    utf8.c \
    regex.c \
    hash_table.c \
    file_utils.c

libbloomlisp_a_CPPFLAGS = \
    -I$(top_srcdir)/include \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

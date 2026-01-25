# Build static libraries
lib_LIBRARIES = libbloomlisp.a liblineedit.a

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

# Line editing library (for REPL)
liblineedit_a_SOURCES = \
    lineedit.c

liblineedit_a_CPPFLAGS = \
    -I$(top_srcdir)/include \
    $(GC_CFLAGS)

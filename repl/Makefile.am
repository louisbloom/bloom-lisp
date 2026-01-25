# Build REPL executable
bin_PROGRAMS = bloom-repl

bloom_repl_SOURCES = main.c

bloom_repl_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

bloom_repl_LDADD = \
    $(top_builddir)/src/libbloomlisp.a \
    $(top_builddir)/src/liblineedit.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

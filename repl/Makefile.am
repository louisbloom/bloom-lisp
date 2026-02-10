# Build REPL executable
bin_PROGRAMS = bloom-repl

bloom_repl_SOURCES = main.c repl_app.c

bloom_repl_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS) \
    $(BOBA_CFLAGS)

bloom_repl_LDADD = \
    $(top_builddir)/src/libbloomlisp.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS) \
    $(BOBA_LIBS)

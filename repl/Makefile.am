# Build REPL executable
bin_PROGRAMS = bloom-repl

if HAVE_BLOOM_BOBA
bloom_repl_SOURCES = main.c repl_app.c
else
bloom_repl_SOURCES = main.c
endif

bloom_repl_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

bloom_repl_LDADD = \
    $(top_builddir)/src/libbloomlisp.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

if HAVE_BLOOM_BOBA
bloom_repl_CPPFLAGS += $(BOBA_CFLAGS)
bloom_repl_LDADD += $(BOBA_LIBS)
endif

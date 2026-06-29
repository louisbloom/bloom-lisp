# flare/Makefile.am - Build the flare CLI

bin_PROGRAMS = flare

flare_SOURCES = flare.c

flare_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

# ditty_version.h is generated in $(top_builddir)/src. Force it to exist
# before any flare objects compile.
$(flare_OBJECTS): $(top_builddir)/src/ditty_version.h
$(top_builddir)/src/ditty_version.h:
	$(MAKE) -C $(top_builddir)/src ditty_version.h

flare_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

flare_DEPENDENCIES = \
    $(top_builddir)/src/libditty.a

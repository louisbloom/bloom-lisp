# lat/Makefile.am - Build the lat CLI

bin_PROGRAMS = lat

lat_SOURCES = lat.c

lat_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

# bloom_version.h is generated in $(top_builddir)/src. Force it to exist
# before any lat objects compile.
$(lat_OBJECTS): $(top_builddir)/src/bloom_version.h
$(top_builddir)/src/bloom_version.h:
	$(MAKE) -C $(top_builddir)/src bloom_version.h

lat_LDADD = \
    $(top_builddir)/src/libbloomlisp.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

lat_DEPENDENCIES = \
    $(top_builddir)/src/libbloomlisp.a

bin_PROGRAMS = cued
cued_SOURCES = rip.c cued.c sheet.c

AM_CPPFLAGS = -I$(top_srcdir)/lib @AM_CPPFLAGS@
cued_LDADD = $(LIBCDIO_LIBS) $(LIBCDIOPARANOIA_LIBS) $(LIBSNDFILE_LIBS) $(LIBCDDB_LIBS) $(top_builddir)/lib/libcued.a $(AM_LDFLAGS)

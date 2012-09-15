bin_PROGRAMS = sfcmp
sfcmp_SOURCES = sfcmp.c

AM_CPPFLAGS = -I$(top_srcdir)/lib @AM_CPPFLAGS@
sfcmp_LDADD = $(LIBSNDFILE_LIBS) $(top_builddir)/lib/libcued.a $(AM_LDFLAGS)

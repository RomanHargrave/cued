bin_PROGRAMS = sfcat
sfcat_SOURCES = sfcat.c

AM_CFLAGS = -I$(top_srcdir)/lib @AM_CFLAGS@
sfcat_LDADD = $(LIBSNDFILE_LIBS) $(top_builddir)/lib/libcued.a $(AM_LDFLAGS)

bin_PROGRAMS = sfcat
sfcat_SOURCES = sfcat.c

AM_CPPFLAGS = -I$(top_srcdir)/lib @AM_CPPFLAGS@
sfcat_LDADD = $(LIBSNDFILE_LIBS) $(top_builddir)/lib/libcued.a $(AM_LDFLAGS)

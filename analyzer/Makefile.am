bin_PROGRAMS = sfanal
sfanal_SOURCES = sfanal.c

AM_CFLAGS = -I$(top_srcdir)/lib @AM_CFLAGS@
sfanal_LDADD = $(LIBSNDFILE_LIBS) $(top_builddir)/lib/libcued.a $(AM_LDFLAGS)

bin_PROGRAMS = sfanal
sfanal_SOURCES = sfanal.c

AM_CPPFLAGS = -I$(top_srcdir)/lib @AM_CPPFLAGS@
sfanal_LDADD = $(LIBSNDFILE_LIBS) $(top_builddir)/lib/libcued.a $(AM_LDFLAGS)

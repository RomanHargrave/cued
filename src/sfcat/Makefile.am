bin_PROGRAMS = sfcat
sfcat_SOURCES = sfcat.c

# requires later gcc than the 4.2.1 that comes with FreeBSD 9
#if GCC
#    EXTRA_WARN = -Wno-unused-but-set-variable
#endif

AM_CFLAGS = $(EXTRA_WARN) @AM_CFLAGS@
AM_CPPFLAGS = $(LIBCUED_CPPFLAGS) $(LIBCAULK_CFLAGS) $(LIBSNDFILE_CFLAGS) @AM_CPPFLAGS@
sfcat_LDADD = $(LIBCUED_LIBS)     $(LIBCAULK_LIBS)   $(LIBSNDFILE_LIBS)

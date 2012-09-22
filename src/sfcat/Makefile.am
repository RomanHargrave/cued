bin_PROGRAMS = sfcat
sfcat_SOURCES = sfcat.c

# requires later gcc than the 4.2.1 that comes with FreeBSD 9
#if GCC
#    EXTRA_WARN = -Wno-unused-but-set-variable
#endif

AM_CFLAGS = $(EXTRA_WARN) @AM_CFLAGS@
AM_CPPFLAGS = $(LIBSNDFILE_CFLAGS) $(LIBCUED_CPPFLAGS) @AM_CPPFLAGS@
sfcat_LDADD = $(LIBSNDFILE_LIBS)   $(LIBCUED_LIBS)

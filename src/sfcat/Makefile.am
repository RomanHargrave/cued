bin_PROGRAMS = sfcat
sfcat_SOURCES = sfcat.c

if GCC
    EXTRA_WARN = -Wno-unused-but-set-variable
endif

AM_CFLAGS = $(EXTRA_WARN) @AM_CFLAGS@
AM_CPPFLAGS = $(LIBCUED_CPPFLAGS) @AM_CPPFLAGS@
sfcat_LDADD = $(LIBSNDFILE_LIBS) $(LIBCUED_LIBS)

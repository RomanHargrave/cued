noinst_LIBRARIES = libcued.a
libcued_a_SOURCES = pwsc.c format.c cdio2.c qsc.c cddb2.c \
                    pwsc.h format.h cdio2.h qsc.h cddb2.h \
                    cued.h qsc_types.h

AM_CPPFLAGS = $(LIBCOMMON_CPPFLAGS) $(LIBCDDB_CFLAGS) $(LIBCDIO_CFLAGS) $(LIBCDIOPARANOIA_CFLAGS) @AM_CPPFLAGS@

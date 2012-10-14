cued_config.h: $(top_builddir)/config.h
	echo  '#ifndef __CUED_CONFIG_H__' > cued_config.h
	echo  '#define __CUED_CONFIG_H__' >> cued_config.h
	@SED@ @SED_EXTENDED_RE_FLAG@ -e 's/^([ \t]*#[ \t]*define[ \t]+)/\1CUED_/' $(top_builddir)/config.h >>cued_config.h
	echo '#endif /* #ifndef CUED_CONFIG_H */' >>cued_config.h

BUILT_SOURCES=cued_config.h

noinst_LIBRARIES = libcued.a
libcued_a_SOURCES = pwsc.c format.c unix.c cdio2.c qsc.c cddb2.c opt.c dmalloc.c util.c \
                    pwsc.h format.h unix.h cdio2.h qsc.h cddb2.h opt.h dmalloc.h util.h \
                    cued.h dlist.h qsc_types.h macros.h

AM_CPPFLAGS = $(LIBCDDB_CFLAGS) $(LIBCDIO_CFLAGS) @AM_CPPFLAGS@

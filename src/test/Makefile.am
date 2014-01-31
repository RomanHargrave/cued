bin_PROGRAMS = object
object_SOURCES = test.c

AM_CPPFLAGS = -I$(top_builddir)/lib/firstcls               -I$(top_srcdir)/lib/classc \
                $(LIBCOMMON_CPPFLAGS) @AM_CPPFLAGS@
object_LDADD =  $(top_builddir)/lib/firstcls/libfirstcls.a $(top_builddir)/lib/classc/libclassc.a \
                $(LIBCOMMON_LIBS)

BUILT_SOURCES = ob.h
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = ob.m4 crap.m4
include $(top_srcdir)/lib/firstcls/m4.mk

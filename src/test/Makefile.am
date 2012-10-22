bin_PROGRAMS = object
object_SOURCES = test.c ob.h

AM_CPPFLAGS = -I$(top_srcdir)/lib/classc -I$(top_builddir)/lib/firstcls @AM_CPPFLAGS@
object_LDADD = $(top_builddir)/lib/classc/libclassc.a $(top_builddir)/lib/firstcls/libfirstcls.a

BUILT_SOURCES = ob.h
CLEANFILES=$(BUILT_SOURCES)
include $(top_srcdir)/lib/firstcls/m4.mk

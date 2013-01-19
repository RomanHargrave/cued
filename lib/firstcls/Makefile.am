noinst_LIBRARIES = libfirstcls.a
libfirstcls_a_SOURCES = list.c tree.c string.c common.c

AM_CPPFLAGS = -I$(top_srcdir)/lib/classc @AM_CPPFLAGS@

BUILT_SOURCES = firstcls.h
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = firstcls.m4
include m4.mk

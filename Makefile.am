# lib has to come first
SUBDIRS = lib src

EXTRA_DIST = autogen.sh

cued_config.h: $(top_builddir)/config.h
	echo  '#ifndef __CUED_CONFIG_H__' > cued_config.h
	echo  '#define __CUED_CONFIG_H__' >> cued_config.h
	@SED@ @SED_EXTENDED_RE_FLAG@ -e 's/^([ \t]*#[ \t]*define[ \t]+)/\1CUED_/' $(top_builddir)/config.h >>cued_config.h
	echo '#endif /* #ifndef CUED_CONFIG_H */' >>cued_config.h

BUILT_SOURCES=cued_config.h
CLEANFILES=$(BUILT_SOURCES)

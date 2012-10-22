SUFFIXES = .m4

.m4.h:
	$(M4) $(AM_CPPFLAGS) $< >$@

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([cued], [1.2.0], [hydrologiccycle@gmail.com])
#AC_CONFIG_AUX_DIR([admin])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/Makefile.am])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_PROG([PKG_CONFIG], [pkg-config], [yes], [no])
if test "$PKG_CONFIG" = no; then
    AC_MSG_ERROR([You must install pkg-config so that required libraries can be found.
                  On Solaris, install this with the command "pkg install gettext".])
fi
PKG_CHECK_MODULES([LIBCDIO], [libcdio >= 0.81])
PKG_CHECK_MODULES([LIBCDIOPARANOIA], [libcdio_paranoia >= 0.81])
PKG_CHECK_MODULES([LIBSNDFILE], [sndfile >= 1.0.17])
PKG_CHECK_MODULES([LIBCDDB], [libcddb >= 1.3.0])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdint.h stdlib.h string.h unistd.h cdio/mmc_ll_cmds.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL # requires 2.6.9
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([localtime_r memmove memset mkdir munmap strchr strdup strerror strrchr strtol])


# use AM_CFLAGS in Makefile.am rather than CFLAGS, which is reserved for user
AC_SUBST([AM_CFLAGS])

AM_CONDITIONAL([GCC], [test "$GCC" = yes])

if test "$GCC" = yes; then
    AM_CFLAGS="-Wall -Wstrict-aliasing=3 -Wformat=2 ${AM_CFLAGS}"
    if test x"$CC" = x"g++"; then
        AM_CFLAGS="-std=gnu++98 ${AM_CFLAGS}"
    else
        AM_CFLAGS="-std=gnu99 ${AM_CFLAGS}"
    fi
fi


#AM_PROG_CC_C_O is ugly so use AM_CPPFLAGS instead of per-program CPPFLAGS
AC_SUBST([AM_CPPFLAGS])

AC_ARG_ENABLE([dmalloc_libc], AS_HELP_STRING([--enable-dmalloc-libc],
    [debug malloc that overrides libc (default disabled)]), [
    AC_SEARCH_LIBS([dlsym], [dl dld], [],
        [AC_MSG_ERROR([failed to find dlsym() function needed by --enable-dmalloc-libc])])
    AM_CPPFLAGS="-DOVERRIDE_LIBC ${AM_CPPFLAGS}"
])

AC_ARG_ENABLE([dmalloc_naive], AS_HELP_STRING([--enable-dmalloc-naive],
    [debug malloc that does not override libc (default disabled)]), [
    AM_CPPFLAGS="-D'malloc(x)=dmalloc(x)' -D'calloc(x, y)=dcalloc(x, y)' -D'free(x)=dfree(x)' -D'realloc(x, y)=drealloc(x, y)' ${AM_CPPFLAGS}"
])


LIBCUED_LIBS='$(top_builddir)/lib/cued/libcued.a'
LIBCUED_CPPFLAGS='-I$(top_srcdir)/lib/cued'
AC_SUBST([LIBCUED_LIBS])
AC_SUBST([LIBCUED_CPPFLAGS])


AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/cued/Makefile
                 src/Makefile
                 src/cued/Makefile
                 src/qdump/Makefile
                 src/sfcat/Makefile
                 src/analyzer/Makefile
                 src/sfcmp/Makefile])

#                 src/classc/GNUmakefile
#                 src/firstcls/GNUmakefile
#                 src/test/GNUmakefile])

AC_OUTPUT

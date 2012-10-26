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

# MacOS needs this.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Check for sed and how to run (borrowed from libcdio).
AC_PROG_SED
AC_MSG_CHECKING([Checking what sed uses for extended regular expressions])
SED_EXTENDED_RE_FLAG='-r'
result=$(echo 'define' | $ac_path_SED -r -e 's/^define/foo/' 2>/dev/null)
if [[ $? -ne 0 ]] ; then
    result=$(echo 'define' | $ac_path_SED -r -E 's/^define/foo/' 2>/dev/null)
    if [[ $? -ne 0 ]] ; then
        SED_EXTENDED_RE_FLAG='-E'
    else
        AC_MSG_WARN([Don't have SED that understand extended RE's. Some minor compilation issues may fail.])
        SED_EXTENDED_RE_FLAG=''
    fi
fi
unset result
AC_MSG_RESULT($SED_EXTENDED_RE_FLAG)
AC_SUBST(SED_EXTENDED_RE_FLAG)


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


# Check for newer libcdio MMC header organization.
# MacPorts installs headers in /opt/local so CPPFLAGS magic is needed.
SAVE_CPPFLAGS=${CPPFLAGS}
CPPFLAGS="${LIBCDIO_CFLAGS} ${CPPFLAGS}"
AC_CHECK_HEADERS([cdio/mmc_ll_cmds.h])
CPPFLAGS=${SAVE_CPPFLAGS}
unset SAVE_CPPFLAGS


# Check for newer libcdio cdtext interface.
SAVE_LDFLAGS=${LDFLAGS}
LDFLAGS="${LIBCDIO_LIBS} ${LDFLAGS}"
AC_CHECK_FUNC([cdtext_lang2str],
    [AC_DEFINE(HAVE_CDTEXT_V2, [], [Define this if libcdio has the 'cdtext_lang2str' function])])
LDFLAGS=${SAVE_LDFLAGS}
unset SAVE_LDFLAGS


# Check for non-standard C runtime behavior.
AC_MSG_CHECKING([for assignable stdout])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([#include <stdio.h>], [stdout = stderr;])],
    [AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_ASSIGNABLE_STDOUT, [], [Define this if stdout can be assigned])],
    [AC_MSG_RESULT(no)])


# Check for GCC and set dialect and warnings flags.

# Use AM_CFLAGS in Makefile.am rather than CFLAGS, which is reserved for user.
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


# Checks for options to configure.

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

AC_ARG_ENABLE([classc], AS_HELP_STRING([--enable-classc],
    [object-oriented extension to the C programming language (default disabled)]))
AM_CONDITIONAL([CLASSC], test "x$enable_classc" = "xyes")
AS_IF([test "x$enable_classc" = "xyes"], [

    # Need m4 for classc.
    AC_PATH_PROG([M4], [m4], [no])
    if test "$M4" = no; then
        AC_MSG_ERROR([Classc needs GNU m4 installed.])
    fi

    AC_MSG_CHECKING([for m4 that accepts -g flag])
    result=$(echo foo | $M4 -g 2>/dev/null)
    if [[ $? -eq 0 ]] ; then
        AM_M4FLAGS='-g'
        AC_MSG_RESULT([yes])
    else
        AC_MSG_RESULT([no])
    fi
    unset result
    AC_SUBST([AM_M4FLAGS])
])


# Static library shared by various binaries.

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
                 src/sfcmp/Makefile
                 lib/classc/Makefile
                 lib/firstcls/Makefile
                 src/test/Makefile])

AC_OUTPUT

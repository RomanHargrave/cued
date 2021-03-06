#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([cued], [1.2.0], [hydrologiccycle@gmail.com])
#AC_CONFIG_AUX_DIR([admin])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/Makefile.am])
AC_CONFIG_HEADERS([config.h])


# Checks for programs.
#

AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# MacOS needs this for its non-POSIX ar.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Check for sed and how to run (borrowed from libcdio).
AC_PROG_SED
AC_MSG_CHECKING([Checking what sed uses for extended regular expressions])
SED_EXTENDED_RE_FLAG='-r'
result=$(echo 'define' | $SED -r -e 's/^define/foo/' 2>/dev/null)
if [[ $? -ne 0 ]] ; then
    result=$(echo 'define' | $SED -E -e 's/^define/foo/' 2>/dev/null)
    if [[ $? -eq 0 ]] ; then
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
#

AC_CHECK_PROG([PKG_CONFIG], [pkg-config], [yes], [no])
if test "$PKG_CONFIG" = no; then
    AC_MSG_ERROR([You must install pkg-config so that required libraries can be found.
                  On Solaris, install this with the command "pkg install gettext".])
fi
PKG_CHECK_MODULES([LIBCDIO], [libcdio >= 0.81])
PKG_CHECK_MODULES([LIBSNDFILE], [sndfile >= 1.0.17])
PKG_CHECK_MODULES([LIBCDDB], [libcddb >= 1.3.0])
PKG_CHECK_MODULES([LIBCAULK], [libcaulk >= 1.0.0])

# MacPorts installs headers in /opt/local so CPPFLAGS magic is needed.
# Bracket package header checks with save and restore of CPPFLAGS.
SAVE_CPPFLAGS=${CPPFLAGS}
CPPFLAGS="${LIBCDIO_CFLAGS} ${CPPFLAGS}"

    AC_ARG_WITH([paranoia], AS_HELP_STRING([--without-paranoia],
        [paranoia (-p) support (default with)]))
    AS_IF([test "x$with_paranoia" != "xno"], [
        PKG_CHECK_MODULES([LIBCDIOPARANOIA], [libcdio_paranoia >= 0.81], [
            AC_DEFINE([HAVE_PARANOIA], [], [Define this if paranoia is installed])
            AC_CHECK_HEADERS([cdio/paranoia/paranoia.h])
        ], [
            AC_MSG_WARN([Paranoia was not found; building without it.])
        ])
    ])

    # Check for newer libcdio MMC header organization.
    AC_CHECK_HEADERS([cdio/mmc_ll_cmds.h])

CPPFLAGS=${SAVE_CPPFLAGS}
unset SAVE_CPPFLAGS


# Check for newer libcdio cdtext interface.
SAVE_LIBS=${LIBS}
LIBS="${LIBCDIO_LIBS} ${LIBS}"
AC_CHECK_FUNC([cdtext_lang2str],
    [AC_DEFINE([HAVE_CDTEXT_V2], [], [Define this if libcdio has the 'cdtext_lang2str' function])])
LIBS=${SAVE_LIBS}
unset SAVE_LIBS


# Check for non-standard C runtime behavior.
AC_MSG_CHECKING([for assignable stdout])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>], [stdout = stderr;])], [
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_ASSIGNABLE_STDOUT], [], [Define this if stdout can be assigned])
], [
    AC_MSG_RESULT(no)
])

# Check for PATH_MAX in limits.h.
AC_MSG_CHECKING([for PATH_MAX in limits.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <limits.h>], [int x = PATH_MAX;])], [
    AC_MSG_RESULT(yes)
    AC_DEFINE([PATH_MAX], [PATH_MAX])
], [
    AC_MSG_RESULT(no)
    AC_DEFINE([PATH_MAX], [8192], [Define this to be the maximum path length])
])


# Check for GCC and set dialect and warnings flags.

# Use AM_CFLAGS in Makefile.am rather than CFLAGS, which is reserved for user.
AC_SUBST([AM_CFLAGS])

AM_CONDITIONAL([GCC], [test "$GCC" = yes])

if test "$GCC" = yes; then
    AM_CFLAGS="-Wall -Wformat=2 ${AM_CFLAGS}"
    if test "x$CC" != "xclang" && test "x$CC" != "xclang++"; then
        AM_CFLAGS="-Wstrict-aliasing=3 ${AM_CFLAGS}"
    fi
    if test "x$CC" = "xg++" || test "x$CC" = "xclang++"; then
        AM_CFLAGS="-std=gnu++98 ${AM_CFLAGS}"
    else
        AM_CFLAGS="-std=gnu99 ${AM_CFLAGS}"
    fi
fi


# Add common header files.
AC_SUBST([AM_CPPFLAGS])
AM_CPPFLAGS='-I$(top_builddir)/include -I$(top_srcdir)/include'


# Static library shared by various binaries.
LIBCUED_LIBS='$(top_builddir)/lib/cued/libcued.a'
LIBCUED_CPPFLAGS='-I$(top_srcdir)/lib/cued'
AC_SUBST([LIBCUED_LIBS])
AC_SUBST([LIBCUED_CPPFLAGS])


# Always generate all Makefiles because AM_CONDITIONAL in SUBDIRS
# is ignored during make maintainer-clean
AC_CONFIG_FILES([Makefile
                 include/Makefile
                 include/cued/Makefile
                 lib/Makefile
                 lib/cued/Makefile
                 src/Makefile
                 src/cued/Makefile
                 src/qdump/Makefile
                 src/sfcat/Makefile
                 src/analyzer/Makefile
                 src/sfcmp/Makefile])

AC_OUTPUT

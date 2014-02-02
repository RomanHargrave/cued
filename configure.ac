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

# Check for m4 and how to run.
AC_ARG_ENABLE([classc], AS_HELP_STRING([--disable-classc],
    [object-oriented extension to the C programming language (default enabled)]))

# can use AM_IF instead of AS_IF with later versions of automake
AS_IF([test "x$enable_classc" != "xno"], [
    AC_PATH_PROG([M4], [m4], [no])
    if test "$M4" = no; then
        AC_MSG_WARN([Classc needs m4 installed; disabling classc.])
        enable_classc="no"
    else
        AC_MSG_CHECKING([for m4 that accepts -g flag])
        result=$(echo foo | $M4 -g 2>/dev/null)
        if [[ $? -eq 0 ]] ; then
            AM_M4FLAGS="-g ${AM_M4FLAGS}"
            AC_MSG_RESULT([yes])
        else
            AC_MSG_RESULT([no])
        fi
        unset result
        AC_SUBST([AM_M4FLAGS])
    fi
])

# this has to appear at top-level; otherwise, would specify inside AS_IF
AM_CONDITIONAL([CLASSC], test "x$enable_classc" != "xno")


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
SAVE_LDFLAGS=${LDFLAGS}
LDFLAGS="${LIBCDIO_LIBS} ${LDFLAGS}"
AC_CHECK_LIB([cdio], [cdtext_lang2str],
    [AC_DEFINE([HAVE_CDTEXT_V2], [], [Define this if libcdio has the 'cdtext_lang2str' function])])
LDFLAGS=${SAVE_LDFLAGS}
unset SAVE_LDFLAGS


# Check for mcheck.h.
AC_CHECK_HEADERS([mcheck.h])

# Check for memrchr in libc.
AC_CHECK_FUNC([memrchr],
    [AC_DEFINE([HAVE_MEMRCHR], [], [Define this if libc has the 'memrchr' function])])

# Check for memmem in libc.
AC_CHECK_FUNC([memmem],
    [AC_DEFINE([HAVE_MEMMEM], [], [Define this if libc has the 'memmem' function])])

# Check for non-standard C runtime behavior.
AC_MSG_CHECKING([for assignable stdout])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>], [stdout = stderr;])], [
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_ASSIGNABLE_STDOUT], [], [Define this if stdout can be assigned])
], [
    AC_MSG_RESULT(no)
])

# Check for missing const qualifier in struct option (for Solaris.)
AC_MSG_CHECKING([for const qualifier in struct option])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <getopt.h>], [struct option o; const char *foo = "foo"; o.name = foo;])], [
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_CONST_OPTION], [], [Define this if struct option has the const qualifier])
], [
    AC_MSG_RESULT(no)
])

# Check for variable length arrays (missing in Solaris Studio C++ compiler.)
AC_MSG_CHECKING([for variable length arrays])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [ int a = 5;  int b @<:@ a @:>@; ])], [
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_VAR_ARRAY], [], [Define this if compiler implements variable length arrays])
], [
    AC_MSG_RESULT(no)
])

# Check for constructor priorities (missing in Solaris compilers, including gcc.)
AC_MSG_CHECKING([for constructor priorities])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([ static void foo(void) __attribute__((constructor(101))); ])], [
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_CONSTRUCT_PRIO], [], [Define this if compiler implements constructor priorities])
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


# Checks for options to configure.
#

AC_SUBST([AM_CPPFLAGS])

AC_ARG_ENABLE([dmalloc_libc], AS_HELP_STRING([--enable-dmalloc-libc],
    [debug malloc that overloads libc malloc (default disabled)]))
AS_IF([test "x$enable_dmalloc_libc" = "xyes"], [
    AC_SEARCH_LIBS([dlsym], [dl dld], [
        AC_DEFINE([OVERLOAD_LIBC], [], [Define this if debug malloc should overload libc malloc])
    ], [
        AC_MSG_WARN([Failed to find function 'dlsym' needed by --enable-dmalloc-libc; not enabled])
    ])
])

AC_ARG_ENABLE([dmalloc_naive], AS_HELP_STRING([--enable-dmalloc-naive],
    [debug malloc that does not overload libc (default disabled)]))
AS_IF([test "x$enable_dmalloc_naive" = "xyes"], [
    AM_CPPFLAGS="-D'malloc(x)=dmalloc(x)' -D'calloc(x, y)=dcalloc(x, y)' -D'free(x)=dfree(x)' -D'realloc(x, y)=drealloc(x, y)' ${AM_CPPFLAGS}"
])


# Static library shared by various binaries.
LIBCOMMON_LIBS='$(top_builddir)/lib/common/libcommon.a'
LIBCOMMON_CPPFLAGS='-I$(top_srcdir)/lib/common'
LIBCUED_LIBS='$(top_builddir)/lib/cued/libcued.a'
LIBCUED_CPPFLAGS='-I$(top_srcdir)/lib/cued'
AC_SUBST([LIBCOMMON_LIBS])
AC_SUBST([LIBCOMMON_CPPFLAGS])
AC_SUBST([LIBCUED_LIBS])
AC_SUBST([LIBCUED_CPPFLAGS])


# Always generate classc Makefiles because AM_CONDITIONAL in SUBDIRS
# is ignored during make maintainer-clean
AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/common/Makefile
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

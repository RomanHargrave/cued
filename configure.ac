#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([cued], [1.2.0], [hydrologiccycle@gmail.com])
#AC_CONFIG_AUX_DIR([admin])
AM_INIT_AUTOMAKE([@PACKAGE_NAME@], [@PACKAGE_VERSION@])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
PKG_CHECK_MODULES([LIBCDIO], [libcdio >= 0.81])
PKG_CHECK_MODULES([LIBCDIOPARANOIA], [libcdio_paranoia >= 0.81])
PKG_CHECK_MODULES([LIBSNDFILE], [sndfile >= 1.0.17])
PKG_CHECK_MODULES([LIBCDDB], [libcddb >= 1.3.0])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stdint.h stdlib.h string.h unistd.h cdio/mmc_ll_cmds.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
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

# distribute additional compiler and linker flags among Makefiles
# --> set and change these variables instead of CXXFLAGS or LDFLAGS (for user only)
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 cued/Makefile
                 qdump/Makefile
                 sfcat/Makefile
                 analyzer/Makefile
                 sfcmp/Makefile])

#                 classc/GNUmakefile
#                 firstcls/GNUmakefile
#                 test/GNUmakefile])

AC_OUTPUT
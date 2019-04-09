#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xrun], [0.0.1])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_PROG_CC_STDC([C99])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_SED

# Checks for libraries.

AC_CHECK_LIB(yajl, [yajl_complete_parse, yajl_parse])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_FUNC_FORK
AC_FUNC_WAIT3
AC_CHECK_FUNCS([memchr memset strtol strerror dup2])

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
                  [whether to include debug symbols (default is no)])],
  [debug=$enableval],
  [debug=no]
)

AM_CONDITIONAL([DEBUG], [test x$debug = xyes])

test -z "$SED" && SED=sed
if test "x$debug" = xyes; then
  # Remove all optimization flags from CFLAGS
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-O[0-9s]*//g'`

  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-g[0-9]*//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-g[0-9]*//g'`
  changequote([,])

  CFLAGS="$CFLAGS -g3 -O0 -Wall"
  CXXFLAGS="$CXXFLAGS -g3 -O0 -Wall"
else
  # Remove debug flags
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-g[0-9]*//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-g[0-9]*//g'`
  changequote([,])
fi

AC_CANONICAL_HOST

AM_CONDITIONAL([BUILD_X86], [test $(expr match arch-$host_cpu 'arch-x86_64\|i.*86') -ne 0])
AM_CONDITIONAL([BUILD_ARM], [test $(expr match arch-$host_cpu 'arch-arm.*') -ne 0])
AS_CASE([$host_cpu],
  [x86_64],
  [
    AC_DEFINE([XR_ARCH_X86_64], [], [Build for x64 platform])
    AC_DEFINE([XR_ARCH_X86_IA32], [], [Build for x86 platform])
  ],
  [i*86],
  [
    AC_DEFINE([XR_ARCH_X86_IA32], [], [Build for x86 platform])
  ],
  [arm*],
  [
    AC_DEFINE([XR_ARCH_ARM], [], [Build for arm platform])
  ],
  [*],
  [
    AC_MSG_ERROR([platform no support])
  ]
)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/xrun/Makefile
                 src/xrunc/Makefile
                 tools/arm/Makefile
                 tools/x86/Makefile])

AC_OUTPUT

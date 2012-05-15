AC_PREREQ([2.68])

# Library versioning
# Making releases:
#       micro_version += 1
#       interface_age += 1
# If functions have been added or backward-compatibility has been broken:
#       interface_age = 0
m4_define([major_version], [0])
m4_define([minor_version], [0])
m4_define([micro_version], [1])
m4_define([interface_age], [0])

AC_INIT([rabbitmq-c], [major_version.minor_version.micro_version],
	[https://github.com/rabbitmq/rabbitmq-c/issues], [librabbitmq],
	[http://www.rabbitmq.com/])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.9 subdir-objects foreign -Wno-portability])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_SRCDIR([librabbitmq/amqp.h])
AC_GNU_SOURCE

# Initialize Libtool
LT_PREREQ([2.2])
LT_INIT([disable-static win32-dll])

# Program checks
AC_PROG_CC_C99

# Environment setup
AC_CANONICAL_HOST
AC_C_BIGENDIAN
AC_C_INLINE

# Set compiler flags
AX_TRY_CFLAGS([-Wall], [AX_CFLAGS([-Wall])])
AX_TRY_CFLAGS([-Wextra], [AX_CFLAGS([-Wextra])])
AX_TRY_CFLAGS([-pedantic], [AX_CFLAGS([-pedantic])])
AX_TRY_LDFLAGS([-no-undefined], [NO_UNDEFINED=-no-undefined])
AC_SUBST([NO_UNDEFINED])

# Libtool versioning
LT_CURRENT=m4_eval(minor_version + micro_version - interface_age)
LT_REVISION=interface_age
LT_AGE=m4_eval(m4_eval(minor_version + micro_version) - interface_age)
AC_SUBST([LT_CURRENT])
AC_SUBST([LT_REVISION])
AC_SUBST([LT_AGE])

if test "x$GCC" = "xyes"; then
  dnl Only use -Wall if we have gcc
  if ! echo "$CFLAGS" | grep "\-Wall" 2> /dev/null ; then
    CFLAGS="$CFLAGS -Wall"
  fi
fi

dnl Detect the kind of host we're building for
windows=no
case "${host}" in
*-*-mingw*)
        windows=yes
        ;;
esac
AM_CONDITIONAL(WINDOWS, test "x$windows" = xyes)
AS_IF([test "x$windows" = xyes],
  [AC_DEFINE([WINDOWS], [1], [Define to 1 if on Windows.])]
)

AM_CONDITIONAL(GCC, test "x$GCC" = xyes)

# Detect how to declare inline functions.  Because we will sometimes
# use "-ansi -pedantic" with gcc, we need to make sure the result will
# work in that context.
orig_cflags="$CFLAGS"
AS_IF([test "x$GCC" = "xyes"], [CFLAGS="$CFLAGS -ansi -pedantic"])
CFLAGS="$orig_cflags"

dnl Decide which API abstraction layer to use
PLATFORM_DIR=unix
if test "x$windows" = xyes ; then
   PLATFORM_DIR=windows
fi
AC_SUBST(PLATFORM_DIR)

AM_CONDITIONAL([OS_UNIX], [test "x$windows" = "xno"])
AM_CONDITIONAL([OS_WIN32], [test "x$windows" = "xyes"])

dnl Enable -m64 if we were asked to do so
AC_ARG_ENABLE(64-bit,
[  --enable-64-bit         produce 64-bit library],
[CFLAGS="$CFLAGS -m64"; LDFLAGS="$LDFLAGS -m64"],
)

AC_MSG_CHECKING(finding a python with simplejson installed)
found_python=no
checkPython() {
    if test "$found_python" = "yes"
    then
      return
    fi
    PYTHON=$1
    if $PYTHON -c 'import json' 2>/dev/null \
       || $PYTHON -c 'import simplejson' 2>/dev/null
    then
      found_python=yes
      AC_MSG_RESULT($PYTHON)
    fi
}
checkPython python
checkPython python2.6
checkPython python2.5
if test "$found_python" = "no"
then
  AC_MSG_ERROR(could not find a python that can 'import simplejson')
fi

AC_SUBST(PYTHON)

dnl Decide which extra win32 libs we need, and handle other special
dnl cases when building with the Microsoft compiler
EXTRA_LIBS=
USE_MISINTTYPES=
AS_IF([test "x$windows" = xyes],
  [
    AS_IF([test "x$GCC" = xyes],
          [EXTRA_LIBS="-lws2_32 $EXTRA_LIBS"],
          [
            EXTRA_LIBS="ws2_32.lib $EXTRA_LIBS"
            USE_MSINTTYPES=yes
          ])
  ])

AC_SUBST(EXTRA_LIBS)
AM_CONDITIONAL(USE_MSINTTYPES, test "x$USE_MSINTTYPES" != "x")

dnl Check for libpopt, which we need to build the tools
AC_ARG_WITH([popt],
  [AS_HELP_STRING([--with-popt], [use the popt library.  Needed for tools.])],
  [],
  [with_popt=check])

LIBPOPT=
AS_IF([test "x$with_popt" != xno],
  [AC_CHECK_LIB([popt], [poptGetContext],
    [AC_SUBST([LIBPOPT], ["-lpopt"])
     AC_DEFINE([HAVE_LIBPOPT], [1], [Define if you have libpopt])
    ],
    [if test "x$with_popt" != xcheck; then
       AC_MSG_FAILURE([--with-popt was given, but test for libpopt failed])
     fi
    ])])

AS_IF([test "x$LIBPOPT" != "x"],
  [AC_CHECK_HEADER([popt.h], [],
     [AC_MSG_FAILURE([You have libpopt, but could not find the popt.h header])])
  ])

POPT_CFLAGS=
POPT_LIBS=-lpopt
AC_SUBST([POPT_CFLAGS])
AC_SUBST([POPT_LIBS])

AM_CONDITIONAL(TOOLS, test "x$LIBPOPT" != "x")

AC_ARG_WITH([xmlto],
  [AS_HELP_STRING([--with-xmlto], [use the xmlto toolchain.  Needed for tools man pages.])],
  [],
  [with_xmlto=check])

XMLTO=
AS_IF([test "x$with_xmlto" != xno],
    [AC_CHECK_PROG([XMLTO], [xmlto], [xmlto])
     if test "x$with_xmlto" != xcheck; then
       AC_MSG_FAILURE([--with-xmlto was given, but xmlto not found])
     fi])

AM_CONDITIONAL(DOCS, test "x$XMLTO" != "x")

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
AC_MSG_RESULT([
$PACKAGE_NAME build options:
	Host: $host
	Version: $VERSION
])

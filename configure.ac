AC_INIT([librabbitmq],[0.0.1],[support@rabbitmq.com])
AC_CONFIG_SRCDIR(librabbitmq/codegen.py)
AM_INIT_AUTOMAKE

AC_CONFIG_HEADER([config.h])

dnl Program checks
AC_GNU_SOURCE
AC_PROG_CC

dnl Library checks
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl Header-file checks
AC_HEADER_STDC

if test "x$GCC" = "xyes"; then
  dnl Only use -Wall if we have gcc
  if ! echo "$CFLAGS" | grep "\-Wall" 2> /dev/null ; then
    CFLAGS="$CFLAGS -Wall"
  fi
fi

dnl Detect the kind of host we're building for
AC_CANONICAL_HOST
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
AC_C_INLINE
CFLAGS="$orig_cflags"

AC_C_BIGENDIAN

dnl Decide which API abstraction layer to use
PLATFORM_DIR=unix
if test "x$windows" = xyes ; then
   PLATFORM_DIR=windows
fi
AC_SUBST(PLATFORM_DIR)

dnl Enable -m64 if we were asked to do so
AC_ARG_ENABLE(64-bit,
[  --enable-64-bit         produce 64-bit library],
[CFLAGS="$CFLAGS -m64"; LDFLAGS="$LDFLAGS -m64"],
)

AC_MSG_CHECKING(location of AMQP codegen directory)
for d in ../rabbitmq-codegen codegen ; do
  AMQP_CODEGEN_DIR="$d"
  test -d "$srcdir/$AMQP_CODEGEN_DIR" && break
done
AMQP_SPEC_JSON_PATH="$AMQP_CODEGEN_DIR/amqp-rabbitmq-0.9.1.json"

if test -f "$srcdir/$AMQP_SPEC_JSON_PATH"
then
  AC_MSG_RESULT($AMQP_CODEGEN_DIR)
else
  AC_MSG_ERROR(could not find AMQP spec file at "'$AMQP_SPEC_JSON_PATH'")
fi

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

AC_SUBST(AMQP_CODEGEN_DIR)
AC_SUBST(AMQP_SPEC_JSON_PATH)
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

AM_CONDITIONAL(TOOLS_DOC, test "x$XMLTO" != "x")

AC_OUTPUT(
Makefile
librabbitmq/Makefile
tests/Makefile
examples/Makefile
tools/Makefile
tools/doc/Makefile
)

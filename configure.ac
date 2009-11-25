AC_INIT([librabbitmq],[0.0.1],[tonyg@rabbitmq.com])
AC_CONFIG_SRCDIR(librabbitmq/codegen.py)
AM_INIT_AUTOMAKE

AC_CONFIG_HEADER([config.h])

dnl Program checks
AC_PROG_CC

dnl Library checks
AM_PROG_LIBTOOL

dnl Header-file checks
AC_HEADER_STDC

dnl Only use -Wall if we have gcc
if test "x$GCC" = "xyes"; then
  if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
    CFLAGS="$CFLAGS -Wall"
  fi
fi

dnl Enable -m64 if we were asked to do so
AC_ARG_ENABLE(64-bit,
[  --enable-64-bit         produce 64-bit library],
[CFLAGS="$CFLAGS -m64"; LDFLAGS="$LDFLAGS -m64"],
)

AC_MSG_CHECKING(location of AMQP codegen directory)
sibling_codegen_dir="$ac_abs_confdir/../rabbitmq-codegen"
AMQP_CODEGEN_DIR=$(test -d "$sibling_codegen_dir" && echo "$sibling_codegen_dir" || echo "$ac_abs_confdir/codegen")
AMQP_SPEC_JSON_PATH="$AMQP_CODEGEN_DIR/amqp-0.9.1.json"
if test -f "$AMQP_SPEC_JSON_PATH"
then
  AC_MSG_RESULT($AMQP_CODEGEN_DIR)
else
  AC_MSG_ERROR(could not find AMQP spec file at "'$AMQP_SPEC_JSON_PATH'")
fi

AC_SUBST(AMQP_CODEGEN_DIR)
AC_SUBST(AMQP_SPEC_JSON_PATH)

AC_OUTPUT(
Makefile
librabbitmq/Makefile
tests/Makefile
examples/Makefile
)

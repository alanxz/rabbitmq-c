AC_PREREQ([2.59])

m4_define([major_version], [0])
m4_define([minor_version], [2])
m4_define([micro_version], [0])

# Follow all steps below in order to calculate new ABI version when updating the library
# NOTE: THIS IS UNRELATED to the actual project version
#
# 1. If the library source code has changed at all since the last update, then increment revision
# 2. If any interfaces have been added, removed, or changed since the last update, increment current and set revision to 0.
# 3. If any interfaces have been added since the last public release, then increment age.
# 4. If any interfaces have been removed since the last public release, then set age to 0.
m4_define([soversion_current],   [1])
m4_define([soversion_revision],  [0])
m4_define([soversion_age],       [0])

AC_INIT([rabbitmq-c], [major_version.minor_version.micro_version],
	[https://github.com/alanxz/rabbitmq-c/issues], [librabbitmq],
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
m4_ifdef([AC_PROG_CC_C99], [AC_PROG_CC_C99],
	 [AC_MSG_WARN([Attempt c99 workaround for old versions of autoconf])
	  AC_PROG_CC
	  AX_TRY_CFLAGS([-std=c99], [AX_CFLAGS([-std=c99])])])

# Environment setup
AC_CANONICAL_HOST
AC_C_BIGENDIAN
AC_C_INLINE

# Set compiler flags
AX_TRY_CFLAGS([-Wall], [AX_CFLAGS([-Wall])])
AX_TRY_CFLAGS([-Wextra], [AX_CFLAGS([-Wextra])])
AX_TRY_CFLAGS([-pedantic], [AX_CFLAGS([-pedantic])])
AX_TRY_CFLAGS([-Wstrict-prototypes], [AX_CFLAGS([-Wstrict-prototypes])])
AX_TRY_CFLAGS([-Wcast-align], [AX_CFLAGS([-Wcast-align])])
AX_TRY_CFLAGS([-fno-common], [AX_CFLAGS([-fno-common])])
AX_TRY_CFLAGS([-fvisibility=hidden], [AX_CFLAGS([-fvisibility=hidden])])
AX_TRY_LDFLAGS([-no-undefined], [NO_UNDEFINED=-no-undefined])
AC_SUBST([NO_UNDEFINED])

# Libtool versioning
LT_CURRENT=soversion_current
LT_REVISION=soversion_revision
LT_AGE=soversion_age
AC_SUBST([LT_CURRENT])
AC_SUBST([LT_REVISION])
AC_SUBST([LT_AGE])

# OS setup
os_unix=no
os_win32=no
AS_CASE([$host],
        [*-*-mingw*],
                [os_win32=yes],
        [*],
                [os_unix=yes])
AM_CONDITIONAL([OS_UNIX], [test "x$os_unix" = xyes])
AM_CONDITIONAL([OS_WIN32], [test "x$os_win32" = xyes])

# Extra Win32 setup
AS_IF([test "x$os_win32" = xyes],
      [AC_DEFINE([OS_WIN32], [1], [Define to 1 for Win32.])
       AC_DEFINE([AMQP_BUILD], [1], [Define to 1 for a Win32 build.])
       AS_IF([test "x$GCC" = xyes],
             [AX_LDFLAGS([-lws2_32])],
             [AX_LDFLAGS([ws2_32.lib])])
       AS_IF([test "x$enable_static" = "xyes"],
             [AS_IF([test "x$enable_shared" = "xyes"],
                    [AC_MSG_ERROR([select one of shared @<:@--enable-shared@:>@ or static @<:@--enable-static@:>@.])],
                    [AC_DEFINE([AMQP_STATIC], [1],
                               [Define to 1 for a static Win32 build.])])])])

# 64-bit option
AC_ARG_ENABLE([64-bit],
              [AS_HELP_STRING([--enable-64-bit],
                              [produce 64-bit library @<:@no@:>@])],
              [AX_TRY_CFLAGS([-m64], AX_CFLAGS([-m64]))
               AX_TRY_LDFLAGS([-m64], [AX_LDFLAGS([-m64])])],
              [enable_64_bit=no])

AM_PATH_PYTHON([2.5])

AX_PYTHON_MODULE([json], [Need JSON])
AS_IF([test "x$HAVE_PYMOD_JSON" = "xno"],
      [AX_PYTHON_MODULE([simplejson])])
AS_IF([test "x$HAVE_PYMOD_JSON" = "xno" && test "x$HAVE_PYMOD_SIMPLEJSON" = "xno"],
     [AC_MSG_ERROR([Unable to find json or simplejson python modules])])

AS_VERSION_COMPARE([$PYTHON_VERSION],[3.0],[eval HAVE_PYTHON3=no],[eval HAVE_PYTHON3=yes],[eval HAVE_PYTHON3=yes])

AS_IF([test "x$HAVE_PYTHON3" = "xyes"],
      [AC_PATH_PROG(PYTHON2TO3,[2to3])
      AS_IF([test "x$PYTHON2TO3" = "x"],
            AC_MSG_ERROR([Unable to find 2to3 python utility]))])

AM_CONDITIONAL([PYTHON3], [test "x$HAVE_PYTHON3" = "xyes"])

# Configure AMQP command-line tools
AC_ARG_ENABLE([tools],
	      [AS_HELP_STRING([--enable-tools],
			      [build AMQP command-line tools @<:@auto@:>@])],,
	      [enable_tools=auto])
AS_IF([test "x$enable_tools" != "xno"],
      [AX_LIB_POPT([enable_tools=yes], [enable_tools=no])])
AM_CONDITIONAL([TOOLS], [test "x$enable_tools" = "xyes"])

# Configure command-line tool documentation
AC_ARG_ENABLE([docs],
	      [AS_HELP_STRING([--enable-docs],
			[build command-line tool documentation @<:@auto@:>@])],,
	       [AS_IF([test "x$enable_tools" = "xno"],
		      [enable_docs=no],
		      [enable_docs=auto])])
AC_ARG_VAR([XMLTO], [xmlto command])
AS_IF([test "x$enable_docs" != "xno"],
      [AS_IF([test "x$XMLTO" = "x"],
	     [AC_CHECK_PROGS([XMLTO], [xmlto])])
       AS_IF([test "x$XMLTO" != "x"],
	     [enable_docs=yes],
	     [enable_docs=no])])
AM_CONDITIONAL([DOCS], [test "x$enable_docs" = "xyes"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	librabbitmq.pc
	Makefile
])
AC_OUTPUT
AC_MSG_RESULT([
$PACKAGE_NAME build options:
	Host: $host
	Version: $VERSION
	64-bit: $enable_64_bit
	Tools: $enable_tools
	Documentation: $enable_docs
])

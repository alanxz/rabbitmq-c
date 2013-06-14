# vim:set ts=2 sw=2 sts=2 et:
AC_PREREQ([2.59])

m4_define([major_version], [0])
m4_define([minor_version], [3])
m4_define([micro_version], [0])

# Follow all steps below in order to calculate new ABI version when updating the library
# NOTE: THIS IS UNRELATED to the actual project version
#
# 1. If the library source code has changed at all since the last update, then increment revision
# 2. If any interfaces have been added, removed, or changed since the last update, increment current and set revision to 0.
# 3. If any interfaces have been added since the last public release, then increment age.
# 4. If any interfaces have been removed since the last public release, then set age to 0.
m4_define([soversion_current],   [1])
m4_define([soversion_revision],  [1])
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
PKG_PROG_PKG_CONFIG([0.17])

# Environment setup
AC_CANONICAL_HOST
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
AC_DEFINE([ENABLE_THREAD_SAFETY], [1], [Define to 1 to enable thread safety])

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

# Detect which libraries we might need to use socket functions:
#  Solaris needs -lsocket -lnsl
#  QNX will need -lsocket

AC_SEARCH_LIBS([getaddrinfo], [socket], [],
               [AC_CHECK_LIB([getaddrinfo], [socket],
                             [LIBS="-lsocket -lnsl $LIBS"],
                             [AC_MSG_ERROR([cannot find name resolution library (library with getaddrinfo symbol)])],
                             [-lnsl])])
AC_SEARCH_LIBS([socket], [socket], [],
               [AC_CHECK_LIB([socket], [socket],
                             [LIBS="-lsocket -lnsl $LIBS"], 
                             [AC_MSG_ERROR([cannot find socket library (library with socket symbol)])],
                             [-lnsl])])
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_CHECK_FUNCS([htonll])

AC_ARG_ENABLE([regen-amqp-framing],
              [AS_HELP_STRING([--enable-regen-amqp-framing],
                              [Regenerate amqp_framing.h/amqp_framing.c (for developer use)])])

AM_CONDITIONAL([REGENERATE_AMQP_FRAMING], [test "x$enable_regen_amqp_framing" = "xyes"])

AS_IF([test "x$enable_regen_amqp_framing" = "xyes"],
      [AM_PATH_PYTHON([2.4],,AC_MSG_ERROR([--enable-regen-amqp-framing requires python]))

       AX_PYTHON_MODULE([json], [])
       AS_IF([test "x$HAVE_PYMOD_JSON" = "xno"],
             [AX_PYTHON_MODULE([simplejson])])
       AS_IF([test "x$HAVE_PYMOD_JSON" = "xno" && test "x$HAVE_PYMOD_SIMPLEJSON" = "xno"],
             [AC_MSG_ERROR([Unable to find json or simplejson python modules])])

       AS_VERSION_COMPARE([$PYTHON_VERSION],[3.0],
                          [eval HAVE_PYTHON3=no],
                          [eval HAVE_PYTHON3=yes],
                          [eval HAVE_PYTHON3=yes])

       AS_IF([test "x$HAVE_PYTHON3" = "xyes"],
             [AC_PATH_PROG(PYTHON2TO3,[2to3])
              AS_IF([test "x$PYTHON2TO3" = "x"],
              AC_MSG_ERROR([Unable to find 2to3 python utility]))])

       ],
       [HAVE_PYTHON3=no])
AM_CONDITIONAL([PYTHON3], [test "x$HAVE_PYTHON3" = "xyes"])

# Configure SSL/TLS
AC_ARG_WITH([ssl],
	    [AS_HELP_STRING([--with-ssl=@<:@cyassl/gnutls/no/openssl/polarssl/yes@:>@],
			    [enable SSL/TLS support @<:@default=openssl@:>@])],
	    [AS_CASE([$withval],
		     [yes], [with_ssl=openssl],
		     [*], [with_ssl=$withval])],
	    [with_ssl=openssl])

AS_IF([test "x$with_ssl" = "xcyassl"],
      [PKG_CHECK_MODULES([SSL], [libcyassl],, [with_ssl=no])],
      [test "x$with_ssl" = "xgnutls"],
      [PKG_CHECK_MODULES([SSL], [gnutls],, [with_ssl=no])],
      [test "x$with_ssl" = "xopenssl"],
      [PKG_CHECK_MODULES([SSL], [openssl >= 0.9.8],, [with_ssl=no])],
      [test "x$with_ssl" = "xpolarssl"],
      [AX_LIB_POLARSSL([SSL_CFLAGS=$POLARSSL_CFLAGS
			SSL_LIBS=$POLARSSL_LIBS],
		       [with_ssl=no])],
      [test "x$with_ssl" = "xno"],,
      [AC_MSG_ERROR([unknown SSL/TLS implementation: $with_ssl])])
AM_CONDITIONAL([SSL_CYASSL], [test "x$with_ssl" = "xcyassl"])
AM_CONDITIONAL([SSL_GNUTLS], [test "x$with_ssl" = "xgnutls"])
AM_CONDITIONAL([SSL_OPENSSL], [test "x$with_ssl" = "xopenssl"])
AM_CONDITIONAL([SSL_POLARSSL], [test "x$with_ssl" = "xpolarssl"])
AM_CONDITIONAL([SSL], [test "x$with_ssl" != "xno"])
AS_IF([test "x$with_ssl" != "xno"],
      [AC_DEFINE([WITH_SSL], [1], [Define to 1 if SSL/TLS is enabled.])])

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

# Configure examples
AC_ARG_ENABLE([examples],
	      [AS_HELP_STRING([--enable-examples],
			      [build example code @<:@auto@:>@])],,
	      [enable_examples=yes])
AM_CONDITIONAL([EXAMPLES], [test "x$enable_examples" = "xyes"])

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
	SSL/TLS: $with_ssl
	Tools: $enable_tools
	Documentation: $enable_docs
	Examples: $enable_examples
])

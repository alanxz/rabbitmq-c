# polarssl.m4 - Check for PolarSSL
#
# Copyright 2012 Michael Steinert
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
# THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#serial 1

# _AX_LIB_POLARSSL
# ----------------
# Check for the PolarSSL library and header file. If found the cache variable
# ax_cv_have_polarssl will be set to yes.
AC_DEFUN([_AX_LIB_POLARSSL],
[dnl
ax_cv_have_polarssl=no
_ax_polarssl_h=no
_ax_polarssl_lib=no
AC_ARG_VAR([POLARSSL_CFLAGS],
	   [C compiler flags for PolarSSL, overriding defaults])
AC_ARG_VAR([POLARSSL_LIBS], [linker flags for PolarSSL, overriding defaults])
AC_CHECK_HEADERS([polarssl/ssl.h],
		 [_ax_polarssl_h=yes],,
		 [$POLARSSL_CFLAGS])
AS_IF([test "x$POLARSSL_LIBS" = "x"],
      [AC_CHECK_LIB([polarssl], [entropy_init],
		    [POLARSSL_LIBS=-lpolarssl
		     _ax_polarssl_lib=yes])],
      [_ax_polarssl_cflags=$CFLAGS
       CFLAGS="$POLARSSL_CFLAGS $CFLAGS"
       _ax_polarssl_ldflags=$LDFLAGS
       LDFLAGS="$POLARSSL_LIBS $LDFLAGS"
       AC_MSG_CHECKING([for libpolarssl])
       AC_TRY_LINK([#include <polarssl/entropy.h>],
		   [entropy_init(NULL)],
		   [AC_MSG_RESULT([$POLARSSL_LIBS])
		    _ax_polarssl_lib=yes],
		   [AC_MSG_RESULT([no])])
       CFLAGS=$_ax_polarssl_cflags
       LDFLAGS=$_ax_polarssl_ldflags])
AS_IF([test "x$_ax_polarssl_h" = "xyes" && \
       test "x$_ax_polarssl_lib" = "xyes"],
      [ax_cv_have_polarssl=yes])
])dnl

# AX_LIB_POLARSSL([ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ------------------------------------------------
# Check if PolarSSL is installed. If found the variable ax_have_polarssl will
# be set to yes.
# ACTION-IF-TRUE: commands to execute if PolarSSL is installed
# ACTION-IF-FALSE: commands to execute if PoloarSSL is not installed
AC_DEFUN([AX_LIB_POLARSSL],
[dnl
AC_CACHE_VAL([ax_cv_have_polarssl], [_AX_LIB_POLARSSL])
ax_have_polarssl=$ax_cv_have_polarssl
AS_IF([test "x$ax_have_polarssl" = "xyes"],
      [AC_DEFINE([HAVE_POLARSSL], [1], [Define to 1 if PolarSSL is available.])
       $1], [$2])
])dnl

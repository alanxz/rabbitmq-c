# popt.m4 - Check for Popt
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

# _AX_LIB_POPT
# ------------
# Check for the Popt library and header file. If found the cache variable
# ax_cv_have_popt will be set to yes.
AC_DEFUN([_AX_LIB_POPT],
[dnl
ax_cv_have_popt=no
_ax_popt_h=no
_ax_popt_lib=no
AC_ARG_VAR([POPT_CFLAGS], [C compiler flags for Popt, overriding defaults])
AC_ARG_VAR([POPT_LIBS], [linker flags for Popt, overriding defaults])
AC_CHECK_HEADERS([popt.h],
		 [_ax_popt_h=yes],,
		 [$POPT_CFLAGS])
AS_IF([test "x$POPT_LIBS" = "x"],
      [AC_CHECK_LIB([popt], [poptGetContext],
		    [POPT_LIBS=-lpopt
		     _ax_popt_lib=yes])],
      [_ax_popt_cflags=$CFLAGS
       CFLAGS="$POPT_CFLAGS $CFLAGS"
       _ax_popt_ldflags=$LDFLAGS
       LDFLAGS="$POPT_LIBS $LDFLAGS"
       AC_MSG_CHECKING([for libpopt])
       AC_TRY_LINK([#include <popt.h>],
		   [poptFreeContext(NULL)],
		   [AC_MSG_RESULT([$POPT_LIBS])
		    _ax_popt_lib=yes],
		   [AC_MSG_RESULT([no])])
       CFLAGS=$_ax_popt_cflags
       LDFLAGS=$_ax_popt_ldflags])
AS_IF([test "x$_ax_popt_h" = "xyes" && \
       test "x$_ax_popt_lib" = "xyes"],
      [ax_cv_have_popt=yes])
])dnl

# AX_LIB_POPT([ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ------------------------------------------------
# Check is installed. If found the variable ax_have_popt will be set to yes.
# ACTION-IF-TRUE: commands to execute if Popt is installed
# ACTION-IF-FALSE: commands to execute if Popt is not installed
AC_DEFUN([AX_LIB_POPT],
[dnl
AC_CACHE_VAL([ax_cv_have_popt], [_AX_LIB_POPT])
ax_have_popt=$ax_cv_have_popt
AS_IF([test "x$ax_have_popt" = "xyes"],
      [AC_DEFINE([HAVE_POPT], [1], [Define to 1 if Popt is available.])
       $1], [$2])
])dnl

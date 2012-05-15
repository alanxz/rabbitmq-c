# cflags.m4 - Test and set compiler flags
#
# Copyright 2011, 2012 Michael Steinert
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

# AX_TRY_CFLAGS(FLAG, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# --------------------------------------------------------
# Test a compiler flag is supported.
# FLAG: a compiler flag to try
# ACTION-IF-TRUE: commands to execute if FLAG is supported
# ACTION-IF-FALSE: commands to execute if FLAG is not supported
AC_DEFUN([AX_TRY_CFLAGS],
[dnl
AC_REQUIRE([AC_PROG_CC])
_ax_cflags=$CFLAGS
CFLAGS="$1 $CFLAGS"
AC_MSG_CHECKING([if compiler accepts '$1'])
AC_TRY_COMPILE([], [],
	       [AC_MSG_RESULT([yes])
		CFLAGS=$_ax_cflags
		$2],
	       [AC_MSG_RESULT([no])
		CFLAGS=$_ax_cflags
		$3])
])dnl

# AX_CFLAGS(FLAGS)
# ----------------
# Enable compiler flags.
# FLAGS: a whitespace-separated list of compiler flags to set
AC_DEFUN([AX_CFLAGS],
[dnl
m4_foreach_w([_ax_flag], [$1],
	     [AS_CASE([" $CFLAGS "],
		      [*[[\ \	]]_ax_flag[[\ \	]]*],
		      [],
		      [*],
		      [CFLAGS="$CFLAGS _ax_flag"])])
])dnl

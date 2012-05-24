# ldflags.m4 - Test and set linker flags
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

# AX_TRY_LDFLAGS(FLAG, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------
# Test if a linker flag is supported.
# FLAG: a linker flag to try
# ACTION-IF-TRUE: commands to execute if FLAG is supported
# ACTION-IF-FALSE: commands to execute if FLAG is not supported
AC_DEFUN([AX_TRY_LDFLAGS],
[dnl
_ax_ldflags=$LDFLAGS
LDFLAGS="$1 $LDFLAGS"
AC_MSG_CHECKING([if linker accepts '$1'])
AC_TRY_LINK([], [],
	    [AC_MSG_RESULT([yes])
	     LDFLAGS=$_ax_ldflags
	     $2],
	    [AC_MSG_RESULT([no])
	     LDFLAGS=$_ax_ldflags
	     $3])
])dnl

# AX_LDFLAGS(flags)
# -----------------
# Enable linker flags.
# FLAGS: a whitespace-separated list of linker flags to set
AC_DEFUN([AX_LDFLAGS],
[dnl
m4_foreach_w([_ax_flag], [$1],
	     [AS_CASE([" $LDFLAGS "],
		      [*[[\ \	]]_ax_flag[[\ \	]]*],
		      [],
		      [*],
		      [LDFLAGS="$LDFLAGS _ax_flag"])])
])dnl

# This file is part of Thunder.

# Copyright (C) 2017 Marc Nieper-Wißkirchen.

# Thunder is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


AC_PREREQ([2.69])
AC_INIT([Thunder], [0.1], [incoming+nieper/thunder@gitlab.com], [],
	[https://gitlab.com/nieper/thunder])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([
  gnits
  1.15
])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_LEX
AC_PROG_YACC
gl_EARLY
# XXX
#AX_COMPILER_VENDOR # FIXME: The code below should only be of interest for the user.
#AS_IF([test "x$ax_cv_c_compiler_vendor" = "xgnu"], [
#  AC_CHECK_TOOLS([AR], [gcc-ar ar], [:])
#  AC_CHECK_TOOLS([NM], [gcc-nm nm], [:])
#  AC_CHECK_TOOLS([RANLIB], [gcc-ranlib ranlib], [:])
#  AM_CFLAGS="$(AM_CFLAGS) -flto"
#])
AM_PROG_AR
LT_PREREQ([2.4.2])
LT_INIT([dlopen])

# Checks for libraries.
AC_CHECK_LIB(gmp, __gmpz_init, ,
  [AC_MSG_ERROR([GNU MP not found, see https://gmplib.org/])])

# For gnulib.
gl_INIT

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
#XXX gl_LIBUNISTRING

AC_CONFIG_FILES([Makefile
                 lib/Makefile
		 common/Makefile
		 include/Makefile
		 libvm/Makefile
		 src/Makefile
		 tests/Makefile])
AC_OUTPUT

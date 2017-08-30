# This file is part of Thunder.

# Copyright (C) 2017 Marc Nieper-Wißkirchen.

# Thunder is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/lib -I$(top_srcdir)/common
AM_LDFLAGS = -dlopen self

LDADD = $(top_srcdir)/common/libvmcommon.la

TEST_EXTENSIONS =
LOG_COMPILER = $(LIBTOOL) --mode=execute $(VALGRIND) --suppressions=$(top_builddir)/lib/malloca.valgrind
TESTS_ENVIRONMENT = @LOCALCHARSET_TESTS_ENVIRONMENT@

check_PROGRAMS = compiler deque image number object symbol_table	\
reader runtime stack write gc
check_SCRIPTS = test.sh
TESTS = $(check_PROGRAMS) $(check_SCRIPTS)

test.sh:
	echo "$(LIBTOOL) --mode=execute $(VALGRIND) ../src/thunder --version | grep \"Thunder\"" > $@
	chmod a+x $@

CLEANFILES = test.sh

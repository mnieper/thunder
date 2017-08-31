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


# TODO(XXX): Add dependency on .ok files.


THUNDER = $(top_srcdir)/src/thunder

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/lib	\
-I$(top_srcdir)/common -I$(top_srcdir)/lightning/include
AM_LDFLAGS = -dlopen self

LDADD = $(top_srcdir)/common/libvmcommon.la

TEST_EXTENSIONS = .tst
LOG_COMPILER = $(LIBTOOL) --mode=execute $(VALGRIND) --suppressions=$(top_builddir)/lib/malloca.valgrind
TST_LOG_COMPILER = ./check.sh
TESTS_ENVIRONMENT = @LOCALCHARSET_TESTS_ENVIRONMENT@

check_PROGRAMS = compiler deque image number object symbol_table	\
reader runtime stack write gc

compiler_SOURCES = compiler.c macros.h

deque_SOURCES = deque.c macros.h

image_SOURCES = image.c macros.h

number_SOURCES = number.c macros.h

object_SOURCES = object.c macros.h

symbol_table_SOURCES = symbol_table.c macros.h

reader_SOURCES = reader.c macros.h

runtime_SOURCES = runtime.c macros.h

stack_SOURCES = stack.c macros.h

write_SOURCES = write.c macros.h

gc_SOURCES = gc.c macros.h

check_SCRIPTS = test.sh check.sh

grep_TEST = test.sh
base_TESTS = hello.tst

$(base_TESTS): check.sh

TESTS = $(check_PROGRAMS) $(grep_TEST) $(base_TESTS)

test.sh: Makefile.am
	echo "$(LIBTOOL) --mode=execute $(VALGRIND) ../src/thunder --version | grep \"Thunder\"" > $@
	chmod a+x $@

check.sh: Makefile.am
	echo "#! /bin/sh" > $@
	echo "$(THUNDER) \"\$$1\" 2>&1 | grep -q \"\`cat \$$(basename \"\$$1\" .tst).ok\`\"" > $@
	chmod a+x $@

EXTRA_DIST = $(base_TESTS) $(base_TESTS:.tst=.ok)

CLEANFILES = $(check_SCRIPTS)

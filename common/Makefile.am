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

AM_YFLAGS = -d -t -Dparse.trace
AM_LFLAGS = -d

BUILT_SOURCES = reader.h scan.c

COMPILER_SOURCES = compiler.c compiler/common.h compiler/compiler.c	\
	compiler/compiler.h compiler/block.c compiler/block.h		\
	compiler/instruction.c compiler/instruction.h			\
	compiler/opcodes.def compiler/parser.c compiler/parser.h	\
	compiler/opcode.c compiler/opcode.h compiler/program.c		\
	compiler/program.h compiler/variable.c compiler/variable.h

noinst_LTLIBRARIES = libvmcommon.la
libvmcommon_la_SOURCES = $(COMPILER_SOURCES) array.h array.c		\
	assembly.c assembly.h deque.c deque.h dump.c	\
	gc.c init.c number.c load.c object.c object-stack.c		\
	resource.c runtime.c stack.c symbol_table.c vector.c write.c	\
	xaligned_alloc.c reader.y scan.l deque.h stack.h vmcommon.h	\
	vector.c vector.h set.h list.c list.h set.h set.c bitset.c	\
	bitset.h heap.h object.h
libvmcommon_la_CPPFLAGS = -I$(top_builddir)/lib			\
	-I$(top_srcdir)/include -I$(top_srcdir)/lightning/include
libvmcommon_la_LIBADD = $(LIBLTDL) $(LTLIBINTL) $(LTLIBICONV)	\
	$(top_builddir)/lightning/lib/liblightning.la		\
	$(top_builddir)/lib/libgnu.la -lgmp -lmpfr -lmpc -lm

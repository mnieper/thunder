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


ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lightning lib common include libvm src doc tests 

EXTRA_DIST = \
	README-dev \
	thunder.x \
	m4/gnulib-cache.m4

CLEANFILES =

if !GIT_CROSS_COMPILING
man_MANS = thunder.1
EXTRA_DIST += $(man_MANS)
endif

if BUILD_FROM_GIT
thunder.1: src/thunder
	$(HELP2MAN) --include=$(top_srcdir)/man/thunder.x $(top_builddir)/src/thunder -o $@-t
	chmod a=r $@-t
	mv -f $@-t $@

CLEANFILES += $(man_MANS)
endif

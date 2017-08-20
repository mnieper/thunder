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


AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_builddir)/lib

bin_PROGRAMS = thunder
thunder_SOURCES = main.c
thunder_LDADD = $(LTLIBINTL) ../libvm/libthunder.la

/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>

#include "vmcommon.h"
#include "macros.h"

int
main (int argc, char *argv)
{
  Heap heap;
  heap_init (&heap, 1ULL << 20);

  SymbolTable symbol_table = heap.symbol_table;

  Object sym1 = make_symbol (&heap, u8"sym1", strlen (u8"sym1"));
  Object sym2 = make_symbol (&heap, u8"sym2", strlen (u8"sym2"));
  Object sym3 = make_symbol (&heap, u8"sym1", strlen (u8"sym1"));
  
  ASSERT (sym1 != sym2);
  ASSERT (sym1 == sym3);
  
  heap_destroy (&heap);
}

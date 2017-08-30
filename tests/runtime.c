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
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "runtime.h"
#include "vmcommon.h"
#include "macros.h"

int
main (int argc, char *argv)
{
  init ();
  
  Heap heap;
  heap_init (&heap, 1ULL << 20);

  Object p = cons (&heap,
		   cons (&heap, make_char ('a'), make_char ('b')),
		   make_char ('c'));
  ASSERT (!is_list (p));

  p = cons (&heap, make_char ('a'),
	    cons (&heap, make_char ('b'), make_null ()));
  ASSERT (is_list (p));
  ASSERT (length (p) == 2);

  ASSERT (is_list (make_null ()));
  ASSERT (length (make_null ()) == 0);

  p = exact_number (&heap, -8, 2);
  ASSERT (fixnum (p) == -4);

  ASSERT (length (list (&heap, make_char ('a'), make_char ('b'))) == 2);
  
  heap_destroy (&heap);
}

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
#include <gmp.h>
#include <stddef.h>

#include "vmcommon.h"
#include "macros.h"

int
main (int argc, char *argv)
{
  init ();
  
  Heap heap;
  heap_init (&heap, 1ULL << 24);

  Object r[4];
  
  Object s = SYMBOL(QUOTE);
  
  Object p = cons (&heap,
		   make_char ('a'),
		   cons (&heap,
			 make_char ('b'),
			 cons (&heap,
			       make_char ('c'),
			       make_char ('d'))));

  r[0] = s; r[1] = p;
  collect (&heap, r, 2);
  s = r[0]; p = r[1];
  
  Object q = cons (&heap,
		   make_char ('e'),
		   p);

  r[0] = s; r[1] = q;
  collect (&heap, r, 2);
  s = r[0]; q = r[1];
  
  ASSERT (is_pair (q));

  Object sym1 = make_symbol (&heap, u8"sym1", strlen (u8"sym1"));
  Object sym2 = make_symbol (&heap, u8"sym2", strlen (u8"sym2"));
  Object sym3 = make_symbol (&heap, u8"sym1", strlen (u8"sym1"));

  r[0] = sym1; r[1] = sym2; r[2] = sym3; r[3] = s;
  collect (&heap, r, 4);
  sym1 = r[0]; sym2 = r[1]; sym3 = r[2]; s = r[3];

  ASSERT (sym1 != sym2);
  ASSERT (sym1 == sym3);

  ASSERT (s == SYMBOL(QUOTE));
  ASSERT (s == make_symbol (&heap, u8"quote", strlen (u8"quote")));

  mpq_t num;
  mpq_init (num);
  p = make_exact_number (&heap, num);
  r[0] = p;
  collect (&heap, r, 1);
  p = r[0];
  mpq_set_si (num, 2, 1);
  q = make_exact_number (&heap, num);
  mpq_t *z;
  z = exact_number_value (p);
  ASSERT (mpq_cmp_si (*z, 0, 1) == 0);  
  mpq_clear (num);

  heap_destroy (&heap);
}

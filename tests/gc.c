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
  Heap heap;
  heap_init (&heap, 1ULL << 24);

  Object s = SYMBOL(QUOTE);
  
  Object p = cons (&heap,
		   make_char ('a'),
		   cons (&heap,
			 make_char ('b'),
			 cons (&heap,
			       make_char ('c'),
			       make_char ('d'))));
  
  collect (&heap, 1ULL << 20, (Object *[2]) {&s, &p}, 2);

  Object q = cons (&heap,
		   make_char ('e'),
		   p);
  
  collect (&heap, 1ULL << 20, (Object *[2]) {&s, &q}, 2);

  ASSERT (is_pair (q));

  Object sym1 = make_symbol (&heap, u8"sym1", strlen (u8"sym1"));
  Object sym2 = make_symbol (&heap, u8"sym2", strlen (u8"sym2"));
  Object sym3 = make_symbol (&heap, u8"sym1", strlen (u8"sym1"));
  
  collect (&heap, 1ULL << 20, (Object *[4]) {&sym1, &sym2, &sym3, &s}, 4);

  ASSERT (sym1 != sym2);
  ASSERT (sym1 == sym3);

  ASSERT (s == SYMBOL(QUOTE));
  ASSERT (s == make_symbol (&heap, u8"quote", strlen (u8"quote")));

  mpq_t num;
  mpq_init (num);
  p = make_exact_number (&heap, num);
  collect (&heap, 1ULL << 20, (Object *[1]) {&p}, 1);
  mpq_set_si (num, 2, 1);
  q = make_exact_number (&heap, num);
  exact_number_value (num, p);
  ASSERT (mpq_cmp_si (num, 0, 1) == 0);  
  mpq_clear (num);
  
  heap_destroy (&heap);
}

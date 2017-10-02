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

#include "deque.h"
#include "macros.h"

typedef struct entry Entry;
struct entry
{
  int value;
  DequeEntry entries;
};
#define ENTRIES DEQUE(struct entry, entries)

int
main (int argc, char *argv)
{

  Deque q;
  deque_init (ENTRIES, &q);

  ASSERT (deque_empty (ENTRIES, &q));

  Entry e1 = { .value = 1 };
  deque_insert_last (ENTRIES, &q, &e1);
  ASSERT (!deque_empty (ENTRIES, &q));

  ASSERT (deque_last (ENTRIES, &q)->value == 1);

  Deque p = deque_initializer (ENTRIES, p);
  deque_concat (ENTRIES, &p, &q);
  ASSERT (deque_empty (ENTRIES, &q));
  ASSERT (!deque_empty (ENTRIES, &p));

  ASSERT (deque_last (ENTRIES, &p)->value == 1);

  ASSERT (deque_pop_first (ENTRIES, &p) == &e1);
  ASSERT (deque_empty (ENTRIES, &p));

  Entry e2 = { .value = 2 };
  deque_insert_last (ENTRIES, &p, &e1);
  deque_insert_last (ENTRIES, &p, &e2);

  {
    int order[] = { 2, 1 };
    size_t i = 0;
    deque_foreach_reverse (e, ENTRIES, &p)
      ASSERT (e->value == order[i++]);
  }
}

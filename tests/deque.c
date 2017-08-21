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

int
main (int argc, char *argv)
{
  typedef struct entry Entry;
  struct entry
  {
    int value;
    DEQUE_ENTRY(entry);
  };

  DEQUE(entry) q;
  deque_init (&q);
  
  ASSERT (deque_is_empty (&q));

  Entry e1 = { .value = 1 };
  deque_insert (&q, &e1);
  ASSERT (!deque_is_empty (&q));

  DEQUE(entry) p;
  deque_init (&p);
  deque_concat (&p, &q);
  ASSERT (deque_is_empty (&q));
  ASSERT (!deque_is_empty (&p));

  ASSERT (deque_pop (&p) == &e1);
  ASSERT (deque_is_empty (&p));
}

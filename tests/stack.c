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

#include "stack.h"
#include "macros.h"

int
main (int argc, char *argv)
{
  typedef struct entry Entry;
  struct entry
  {
    int value;
    STACK_ENTRY(entry);
  };

  STACK(entry) s;
  stack_init (&s);
  
  ASSERT (stack_is_empty (&s));

  Entry e1 = { .value = 1 };
  stack_push (&s, e1);
  ASSERT (!stack_is_empty (&s));

  ASSERT (stack_top (&s).value == e1.value);
  ASSERT (stack_pop (&s).value == e1.value);
  ASSERT (stack_is_empty (&s));

  for (int i = 0; i < 100; ++i)
    stack_push (&s, (Entry) { .value = i });
  for (int i = 99; i >= 0; i--)
    ASSERT (stack_pop (&s).value == i);
  ASSERT (stack_is_empty (&s));
  
  stack_destroy (&s);
}

/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "deque.h"

void
deque_impl_init (struct deque *deque)
{
  deque->first = NULL;
  deque->last = NULL;
}

struct
deque_entry *deque_impl_first (struct deque *deque)
{
  return deque->first;
}

struct
deque_entry *deque_impl_last (struct deque *deque)
{
  return deque->last;
}

struct
deque_entry *deque_impl_next (struct deque_entry *entry)
{
  return entry->next;
}

struct
deque_entry *deque_impl_previous (struct deque_entry *entry)
{
  return entry->prev;
}

bool
deque_impl_empty (struct deque const *deque)
{
  return deque->first == NULL;
}

void
deque_impl_insert_last (struct deque *deque, struct deque_entry *entry)
{
  entry->next = NULL;
  if (deque->first == NULL)
    {
      entry->prev = NULL;
      deque->first = deque->last = entry;
    }
  else
    {
      entry->prev = deque->last;
      deque->last->next = entry;
      deque->last = entry;
    }
}

void
deque_impl_concat (struct deque *deque1, struct deque *deque2)
{
  if (deque_impl_empty (deque2))
    return;
  if (deque_impl_empty (deque1))
    *deque1 = *deque2;
  else
    {
      deque1->last = deque2->last;
      deque1->last->next = deque2->first;
      deque2->first->prev = deque1->last;
    }
  deque_impl_init (deque2);
}

void
deque_impl_remove (struct deque *deque, struct deque_entry *entry)
{
  if (entry->prev != NULL)
    entry->prev->next = entry->next;
  else
    deque->first = entry->next;

  if (entry->next != NULL)
    entry->next->prev = entry->prev;
  else
    deque->last = entry->prev;
}

struct deque_entry *
deque_impl_pop_first (struct deque *deque)
{
  if (deque_impl_empty (deque))
    return NULL;
  struct deque_entry *entry = deque_impl_first (deque);
  deque_impl_remove (deque, entry);
  return entry;
}

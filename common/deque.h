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

#ifndef DEQUE_H_INCLUDED
#define DEQUE_H_INCLUDED

#include <stddef.h>

#define DEQUE_ENTRY(name)			\
  struct					\
  {						\
    struct name *next;                          \
    struct name **prev;				\
  } deque_entries

#define DEQUE(name)				\
  struct 					\
  {						\
    struct name *first;				\
    struct name **last;				\
    struct name *tmp;				\
  }

#define deque_init(deque)			\
  do						\
    {						\
      (deque)->first = NULL;			\
      (deque)->last = &(deque)->first;		\
    }						\
  while (0)

#define deque_insert(deque, entry)			\
  do							\
    {							\
      (entry)->deque_entries.next = NULL;		\
      (entry)->deque_entries.prev = (deque)->last;	\
      *(deque)->last = (entry);				\
      (deque)->last = &(entry)->deque_entries.next;	\
    }							\
  while (0)

#define deque_first(deque)			\
  ((deque)->first)

#define deque_is_empty(deque)			\
  (deque_first (deque) == NULL)

#define deque_concat(deque1, deque2)				\
  do								\
    {								\
      if (!deque_is_empty (deque2))				\
	{	 						\
          *(deque1)->last = (deque2)->first;			\
          (deque2)->first->deque_entries.prev = (deque1)->last;	\
	  (deque1)->last = (deque2)->last;			\
	  deque_init ((deque2));				\
	}							\
    }								\
  while (0)

#define deque_remove(deque, entry)					\
  ((((entry)->deque_entries.next) != NULL)				\
   ? ((entry)->deque_entries.next->deque_entries.prev			\
      = (entry)->deque_entries.prev, 0)					\
   : ((deque)->last = (entry)->deque_entries.prev, 0),			\
   *(entry)->deque_entries.prev = (entry)->deque_entries.next)

#define deque_pop(deque)						\
  (deque_is_empty (deque)						\
   ? NULL								\
   : ((deque)->tmp = deque_first (deque),			\
      (deque_remove (deque, (deque)->tmp)),			\
      (deque)->tmp))

#define deque_destroy(deque)

#endif /* DEQUE_H_INCLUDED */

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
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifndef DEQUE_H_INCLUDED
#define DEQUE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#define DEQUE(type, field) (type, field)

struct deque_entry
{
  struct deque_entry *next;
  struct deque_entry *prev;
};
typedef struct deque_entry DequeEntry;

struct deque
{
  struct deque_entry *first;
  struct deque_entry *last;
};
typedef struct deque Deque;

void deque_impl_init (struct deque *);
struct deque_entry *deque_impl_first (struct deque *);
struct deque_entry *deque_impl_last (struct deque *);
struct deque_entry *deque_impl_next (struct deque_entry *);
struct deque_entry *deque_impl_previous (struct deque_entry *);
bool deque_impl_empty (struct deque const *);
void deque_impl_insert_last (struct deque *, struct deque_entry *);
void deque_impl_concat (struct deque *, struct deque *);
void deque_impl_remove (struct deque *, struct deque_entry *);
struct deque_entry *deque_impl_pop_first (struct deque *);

static inline char *
deque__offset (char *p, ptrdiff_t offset)
{
  if (p == NULL)
    return NULL;
  return p + offset;
}

#define DEQUE__EXPAND(...) __VA_ARGS__
#define DEQUE__FIRST(a, b) a
#define DEQUE__SECOND(a, b) b

#define DEQUE_ENTRY_TYPE(DEQUE) DEQUE__EXPAND(DEQUE__FIRST DEQUE)
#define DEQUE_ENTRY_FIELD(DEQUE) DEQUE__EXPAND(DEQUE__SECOND DEQUE)
#define DEQUE_FIELD(DEQUE, entry)					\
  _Generic ((entry),							\
	    DEQUE_ENTRY_TYPE (DEQUE) *: (&((entry)->DEQUE_ENTRY_FIELD (DEQUE))))
#define DEQUE_ENTRY(DEQUE, entry)					\
  ((DEQUE_ENTRY_TYPE(DEQUE) *)						\
   deque__offset ((char *) entry,					\
		  - offsetof (DEQUE_ENTRY_TYPE(DEQUE),			\
			      DEQUE_ENTRY_FIELD(DEQUE))))

#define deque_initializer(DEQUE, deque) { .first = NULL, .last = NULL }
#define deque_init(DEQUE, deque)		\
  (deque_impl_init (deque))
#define deque_first(DEQUE, deque)		\
  DEQUE_ENTRY (DEQUE, deque_impl_first (deque))
#define deque_last(DEQUE, deque)		\
  DEQUE_ENTRY (DEQUE, deque_impl_last (deque))
#define deque_next(DEQUE, entry)			\
  DEQUE_ENTRY (DEQUE, deque_impl_next (DEQUE_FIELD (DEQUE, entry)))
#define deque_previous(DEQUE, entry)		\
  DEQUE_ENTRY (DEQUE, deque_impl_previous (DEQUE_FIELD (DEQUE, entry)))
#define deque_empty(DEQUE, deque)		\
  (deque_impl_empty (deque))
#define deque_insert_last(DEQUE, deque, entry)	\
  (deque_impl_insert_last (deque, DEQUE_FIELD (DEQUE, entry)))
#define deque_concat(DEQUE, deque1, deque2)	\
  (deque_impl_concat (deque1, deque2))
#define deque_remove(DEQUE, deque, entry)	\
  (deque_impl_remove (deque, DEQUE_FIELD(DEQUE, entry)))
#define deque_pop_first(DEQUE, deque)		\
  DEQUE_ENTRY (DEQUE, deque_impl_pop_first (deque))

#define deque_foreach(var, DEQUE, deque)				\
  for (DEQUE_ENTRY_TYPE(DEQUE) *(var) = deque_first (DEQUE, deque);	\
       (var) != NULL;							\
       (var) = deque_next(DEQUE, (var)))

#define deque_foreach_reverse(var, DEQUE, deque)			\
  for (DEQUE_ENTRY_TYPE(DEQUE) *(var) = deque_last (DEQUE, deque); \
       (var) != NULL;							\
       (var) = deque_previous (DEQUE, (var)))

#define deque_foreach_safe(var, DEQUE, deque)				\
  for (DEQUE_ENTRY_TYPE(DEQUE)						\
	 *(var) = deque_first (DEQUE, deque),				\
	 *deque__tmp = NULL;						\
       (var) != NULL && (deque__tmp = deque_next (DEQUE, (var)), true);	\
       (var) = deque__tmp)

#endif /* DEQUE_H_INCLUDED */

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

#ifndef SET_H_INCLUDED
#define SET_H_INCLUDED
#include <stddef.h>

#include "hash.h"
#include "xalloc.h"

#define DEFINE_SET(Set, Element, set)					\
  typedef struct set Set;						\
  struct set								\
  {									\
    Hash_table *hash;							\
  };									\
  									\
  static inline	Set							\
  set##_create (void)							\
  {									\
    Hash_table *hash = hash_initialize (0, NULL, NULL, NULL, NULL);	\
    if (hash == NULL)							\
      xalloc_die ();							\
    return (Set) { hash };						\
  }									\
  									\
  static inline void							\
  set##_free (Set s)							\
  {									\
    hash_free (s.hash);							\
  }									\
									\
  static inline void							\
  set##_clear (Set s)							\
  {									\
    hash_clear (s.hash);						\
  }									\
  									\
  static inline bool							\
  set##_contains (Set s, const Element *e)				\
  {									\
    return hash_lookup (s.hash, e) != NULL;				\
  }									\
  									\
  static inline bool							\
  set##_adjoin (Set s, const Element *e)				\
  {									\
    int res = hash_insert_if_absent (s.hash, e, NULL);			\
    if (res == -1)							\
      xalloc_die ();							\
    return res == 1;							\
  }									\
									\
  static inline bool							\
  set##_delete (Set s, const Element *e)				\
  {									\
    return hash_delete (s.hash, e) != NULL;				\
  }									\
  									\
  static inline size_t							\
  set##_foreach (Set s, bool (* processor) (Element *, void *), void *data) \
  {									\
    return hash_do_for_each (s.hash, (Hash_processor) processor, data);	\
  }

#endif /* SET_H_INCLUDED */

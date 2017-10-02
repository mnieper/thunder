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

#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED
#include <stdbool.h>
#include <stddef.h>

#include "hash.h"
#include "xalloc.h"

#define DEFINE_ARRAY(Array, Element, array)	\
  struct Array;					\
						\
  typedef struct array##_entry Array##Entry;	\
  struct array##_entry				\
  {						\
    size_t index;				\
    Element value;				\
  };						\
  						\
  static Array *					\
  array##_create (void)					\
  {							\
    Hash_table *table = hash_initialize (0,		\
					 NULL,		\
					 array_hash,	\
					 array_compare,	\
					 free);		\
    if (table == NULL)					\
      xalloc_die ();					\
    return (Array *) table;				\
  }							\
  							\
  static void							\
  array##_free (Array *a)					\
  {								\
    hash_free ((Hash_table *) a);				\
  }								\
  								\
  static Element *							\
  array##_lookup (Array *a, size_t index)				\
  {									\
    Array##Entry *entry							\
      = (Array##Entry *) hash_lookup ((Hash_table *) table, &index);	\
    if (entry == NULL)							\
      return NULL;							\
    return &entry->value;						\
  }									\
									\
  static void								\
  array##_insert (Array *a, size_t index, Element e)			\
  {									\
    Array##Entry *entry = XMALLOC (Array##Entry);			\
    entry->index = index;						\
    entry->value = e;							\
    if (hash_insert ((Hash_table *) table, entry) == NULL)		\
      xalloc_die ();							\
  }

size_t array_hash (size_t const *entry, size_t n);
bool array_compare (size_t const *entry1, size_t const *entry2);

#endif /* ARRAY_H_INCLUDED */

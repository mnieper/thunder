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

#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

#include <stddef.h>

#include "xalloc.h"

#define VECTOR(type)				\
  struct					\
  {						\
    type *base;					\
    ptrdiff_t items;				\
    size_t size;				\
  }

#define vector_init(vector)					\
  do								\
    {								\
      (vector)->size = 16;					\
      (vector)->base = xnmalloc (16, sizeof (*(vector)->base));	\
      (vector)->items = 0;					\
    }								\
  while (0)

#define vector_clear(vector)			\
  ((vector)->items = 0)

#define vector_destroy(vector)			\
  (free ((vector)->base))

#define vector_push(vector, entry)					\
  do									\
    {									\
      if ((vector)->items >= (vector)->size)				\
	{								\
	  (vector)->size *= 2;						\
	  (vector)->base = xnrealloc ((vector)->base,			\
				      (vector)->size,			\
				      sizeof (*(vector)->base));	\
	}								\
      *((vector)->base + (vector)->items++) = (entry);			\
    }									\
  while (0)

#define vector_is_empty(vector)			\
  ((vector)->items == 0)

#define vector_empty(vector)			\
  ((vector)->items == 0)

#define vector_top(vector)			\
  (*((vector)->base + (vector)->items - 1))

#define vector_pop(vector)			\
  (*((vector)->base + --(vector)->items))

#define vector_size(vector)			\
  ((vector)->items)

#define VECTOR_FOREACH(vector, var)				\
  for ((var) = (vector)->base; (var) < (vector)->base + (vector)->items; ++(var))

#endif /* VECTOR_H_INCLUDED */

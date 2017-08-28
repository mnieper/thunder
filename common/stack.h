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

#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

#include <stddef.h>

#include "xalloc.h"

#define STACK_ENTRY(name)			\
  struct					\
  {						\
  } stack_entries;

#define STACK(name)				\
  struct					\
  {						\
    struct name *base;				\
    ptrdiff_t items;				\
    size_t size;				\
  }

#define stack_init(stack)					\
  do								\
    {								\
      (stack)->size = 16;					\
      (stack)->base = xnmalloc (16, sizeof (*(stack)->base));	\
      (stack)->items = 0;					\
    }								\
  while (0)

#define stack_destroy(stack)			\
  (free ((stack)->base))

#define stack_push(stack, entry)				\
  do								\
    {								\
      if ((stack)->items >= (stack)->size)			\
	{							\
	  (stack)->size *= 2;					\
	  (stack)->base = xnrealloc ((stack)->base,		\
				     (stack)->size,		\
				     sizeof (*(stack)->base));	\
	}							\
      *((stack)->base + (stack)->items++) = (entry);		\
    }								\
  while (0)

#define stack_is_empty(stack)			\
  ((stack)->items == 0)

#define stack_top(stack)			\
  (*((stack)->base + (stack)->items - 1))

#define stack_pop(stack)			\
  (*((stack)->base + --(stack)->items))

#endif /* STACK_H_INCLUDED */

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
#include <stddef.h>

#include "obstack.h"
#include "unitypes.h"
#include "vmcommon.h"
#include "xalloc.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free  free

void
object_stack_init (ObjectStack *restrict stack)
{
  obstack_init (&stack->obstack);
  obstack_alignment_mask (&stack->obstack) = ALIGNMENT_MASK;
  stack->base = obstack_finish (&stack->obstack);
}

void
object_stack_clear (ObjectStack *restrict stack)
{
  obstack_free (&stack->obstack, stack->base);
}

void
object_stack_destroy (ObjectStack *restrict stack)
{
  obstack_free (&stack->obstack, NULL);
}

void
object_stack_grow (ObjectStack *restrict stack, Object obj)
{
  obstack_grow (&stack->obstack, &obj, sizeof (obj));
}

void
object_stack_grow0 (ObjectStack *restrict stack)
{
  obstack_1grow (&stack->obstack, 0);
}

void
object_stack_ucs4_grow (ObjectStack *restrict stack, ucs4_t c)
{
  obstack_grow (&stack->obstack, &c, sizeof (c));
}

void
object_stack_utf8_grow (ObjectStack *restrict stack, uint8_t *s, size_t len)
{
  obstack_grow (&stack->obstack, s, len * sizeof (uint8_t));
}

void
object_stack_align (ObjectStack *restrict stack)
{
  size_t size = obstack_object_size (&stack->obstack);
  
  obstack_blank (&stack->obstack, ((size + ALIGNMENT_MASK) & ~ALIGNMENT_MASK) - size);
}

Object
object_stack_finish (ObjectStack *restrict stack)
{
  return (Object) obstack_finish (&stack->obstack);
}

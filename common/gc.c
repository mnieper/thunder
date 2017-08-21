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
#include <string.h>

#include "vmcommon.h"
#include "xalloc.h"

Object symbols[SYMBOL_COUNT];

typedef bool (*MutationProcessor) (Object *, Heap *);

static MutationTable *
mutation_table_create ()
{
  MutationTable *p = hash_initialize (0, NULL, NULL, NULL, NULL);
  if (p == NULL)
    xalloc_die ();
  return p;
}

static void
mutation_table_do_for_each (const MutationTable *table, MutationProcessor processor, void *data)
{
  hash_do_for_each (table, (Hash_processor) processor, data);
}

static void
mutation_table_clear (MutationTable *table)
{
  hash_clear (table);
}

static void
mutation_table_free (MutationTable *table)
{
  hash_free (table);
}

static void
mutation_table_insert (MutationTable *table, Pointer pointer)
{
  if (hash_insert (table, pointer) == NULL)
    xalloc_die ();
}

static Pointer
flip (Heap *restrict heap)
{
  Pointer old_start = heap->start;  
  heap->free = heap->start = xaligned_alloc (2 * WORDSIZE, heap->heap_size);
  return old_start;
}

void
heap_init (Heap *heap, size_t heap_size)
{
  heap->heap_size = heap_size;
  heap->mutation_table = mutation_table_create ();
  symbol_table_init (&heap->symbol_table);
  resource_manager_init (&heap->resource_manager);
  obstack_init (&heap->stack);
  obstack_alignment_mask (&heap->stack) = ALIGNMENT_MASK;
  heap->stack_base = obstack_finish (&heap->stack);
  flip (heap);
#define ENTRY(id, name)			\
  symbols[id] = make_symbol (heap, name, strlen (name));
  SYMBOLS
#undef ENTRY
}

void
heap_destroy (Heap *heap)
{
  obstack_free (&heap->stack, NULL);
  mutation_table_free (heap->mutation_table);
  symbol_table_destroy (&heap->symbol_table);
  resource_manager_destroy (&heap->resource_manager);
  free (heap->start);
}

static bool
is_in_heap (Heap *heap, Pointer object)
{
  return object >= heap->start && object < heap->free;
}

static size_t
free_space (Heap *heap)
{
  return (heap->heap_size - (heap->free - heap->start));
}

static Pointer
copy (Heap *heap, Pointer from)
{
  Pointer to = heap->free;
  size_t size = object_size (from);
  if (size > free_space (heap))
    xalloc_die ();
  heap->free += size;
  
  memcpy (to, from, size * WORDSIZE);
  
  if ((*to & HEADER_TYPE_MASK) == SYMBOL_TYPE)
    to = (Pointer) symbol_table_intern (&heap->symbol_table, (Object) (to + 1), true) - 1;
  
  *from = make_mark (to);

  return to;
}

static Pointer 
forward (Heap *heap, Pointer from)
{
  Pointer from_header = object_header (from);
  Pointer to_header = forwarding_address (*from_header);
  if (to_header == NULL)
    to_header = copy (heap, from_header);
  return to_header - (from_header - from);
}

static void
process (Heap *heap, Object *object)
{
  if (!is_pointer (*object) || is_in_heap (heap, (Pointer) *object))
    return;

  if (is_unmanaged ((Pointer) *object))
    {
      switch (header_type ((Pointer) *object))
	{
#define ENTRY(id, type, init, destroy)					\
	  case TYPE(id):						\
	    resource_manager_mark (id,					\
				   &heap->resource_manager,		\
				   (Resource(id) *) ((Pointer) *object - 1)); \
	    return;
	  RESOURCES
#undef ENTRY
	}
    }

  *object = (Object) forward (heap, (Pointer) *object);
}

static bool
processor (Object *object, Heap *heap)
{
  process (heap, object);
  return true;
}

static void
scan (Heap *heap, Pointer ref)
{
  Pointer start = object_pointers (ref);
  /* Check for a binary object without any pointers. */
  if (start == NULL)
    return;
  
  size_t size = object_size (ref);
  Pointer end = ref + size;

  for (Pointer p = start; p < end; ++p)
    process (heap, p);
}

void
collect (Heap *restrict heap, size_t nursery_size, Object *roots[], size_t root_num)
{
  Pointer old_start = (free_space (heap) < nursery_size / WORDSIZE) ? flip (heap) : NULL;
  Pointer ref = heap->start;
  
  resource_manager_begin_gc (&heap->resource_manager, old_start != NULL);

  if (old_start == NULL)
    mutation_table_do_for_each (heap->mutation_table, processor, heap);
  mutation_table_clear (heap->mutation_table);

  symbol_table_clear (&heap->symbol_table, old_start != NULL);
  
  for (size_t i = 0; i < SYMBOL_COUNT; ++i)
    process (heap, &symbols[i]);
  
  for (size_t i = 0; i < root_num; ++i)
    process (heap, roots[i]);

  for (; ref < heap->free; ref += object_size (ref))
    scan (heap, ref);
  
  if (old_start != NULL)
    free (old_start);

  obstack_free (&heap->stack, heap->stack_base);

  resource_manager_end_gc (&heap->resource_manager);
}

void
mutate (Heap *heap, Pointer slot, Object value)
{
  if (is_pointer (value) && !is_in_heap (heap, (Pointer) value) && is_in_heap (heap, slot))
    mutation_table_insert (heap->mutation_table, slot);
  *slot = value;
}

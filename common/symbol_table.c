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

#include "hash-pjw-bare.h"
#include "unitypes.h"
#include "vmcommon.h"

// XXX: TODO gc
// XXX: TODO default symbols wie quote! (quote, ...) : wo gibt es die? Array !


static size_t
hasher (void const *entry, size_t table_size)
{
  return hash_pjw_bare (symbol_bytes ((Object) entry),
			symbol_length ((Object) entry) * sizeof (uint8_t)) % table_size;  
}

static bool
comparator (void const *entry1, void const *entry2)
{  
  size_t len = symbol_length ((Object) entry1);
  if (len != (symbol_length ((Object) entry2)))
    return false;

  return memcmp (symbol_bytes ((Object) entry1), symbol_bytes ((Object) entry2),
		 len * sizeof (uint8_t)) == 0;
}

void
symbol_table_init (SymbolTable *restrict symbol_table)
{
  if ((symbol_table->nursery_table = hash_initialize (0, NULL, hasher, comparator, NULL)) == NULL)
    xalloc_die ();
  if ((symbol_table->heap_table = hash_initialize (0, NULL, hasher, comparator, NULL)) == NULL)
    xalloc_die ();
}

void
symbol_table_destroy (SymbolTable *restrict symbol_table)
{
  hash_free (symbol_table->nursery_table);
  hash_free (symbol_table->heap_table);
}

Object
symbol_table_clear (SymbolTable *restrict symbol_table, bool major_gc)
{
  hash_clear (symbol_table->nursery_table);
  if (major_gc)
    hash_clear (symbol_table->heap_table);
}


/* When the GC flag is set, the symbol is inserted into the heap table
   if absent there.  When the GC flag is not set, the heap table is
   searched first.  If the symbol is not found there, it is inserted
   into the nursery table if not found there. */

Object
symbol_table_intern (SymbolTable *restrict symbol_table, Object sym, bool gc)
{
  Object old;
  Hash_table *restrict table;
  if (gc)
    table = symbol_table->heap_table;
  else
    {
      old = (Object) hash_lookup (symbol_table->heap_table, (void *) sym);
      if ((void *) old != NULL)
	return old;
      table = symbol_table->nursery_table;
    }
  int res = hash_insert_if_absent (table, (void *) sym, (void const **) &old);
  if (res == -1)
    xalloc_die ();
  if (res == 1)
    return sym;
  return old;
}

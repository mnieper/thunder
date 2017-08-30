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
#include <obstack.h>
#include <string.h>

#include "hash-pjw-bare.h"
#include "unistr.h"
#include "unitypes.h"
#include "vmcommon.h"

/* TODO(XXX): Rename file to symbol-table.c. */

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

/* Well-known symbols */

static Hash_table *well_known_symbols;
ObjectStack symbol_stack;

Object symbols[SYMBOL_COUNT];

static Object
make_well_known_symbol (uint8_t *s)
{
  size_t len = u8_strlen (s);
  object_stack_grow (&symbol_stack, SYMBOL_TYPE | WELL_KNOWN_SYMBOL);
  object_stack_grow (&symbol_stack, (len + 1) * sizeof (uint8_t));
  object_stack_utf8_grow (&symbol_stack, s, len);
  object_stack_grow0 (&symbol_stack);
  object_stack_align (&symbol_stack);
  Object sym = object_stack_finish (&symbol_stack) | POINTER_TYPE;
  if (hash_insert (well_known_symbols, (void *) sym) == NULL)
    xalloc_die ();
  return sym;
}

void init_symbols (void)
{
  if ((well_known_symbols
       = hash_initialize (SYMBOL_COUNT, NULL, hasher, comparator, NULL)) == NULL)
    xalloc_die ();

  object_stack_init (&symbol_stack);
  
#define EXPAND_SYMBOL(id, name)				\
  symbols[SYMBOL_##id] = make_well_known_symbol (name);
# include "symbols.def"
#undef EXPAND_SYMBOL
}

void finish_symbols (void)
{
  hash_free (well_known_symbols);

  object_stack_destroy (&symbol_stack);
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
  /* Check first, whether the symbol is well-known. */
  Object old = (Object) hash_lookup (well_known_symbols, (void *) sym);
  if ((void *) old != NULL)
    return old;
  
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

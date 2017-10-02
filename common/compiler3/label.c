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
#include <stdbool.h>
#include <stddef.h>

#include "bitrotate.h"
#include "hash.h"
#include "compiler/error.h"
#include "compiler/label.h"
#include "xalloc.h"

#define LABEL(p) ((struct label const *) p)

static bool
label_comparator (void const *entry1, void const *entry2)
{
  return LABEL(entry1)->name == LABEL(entry2)->name;
}

static size_t
label_hasher (void const *entry, size_t n)
{
  size_t val = rotr_sz ((size_t) LABEL (entry)->name, 3);
  return val % n;
}

#undef LABEL

void
init_labels (Compiler *compiler)
{
  compiler->labels = hash_initialize (0, NULL, label_hasher, label_comparator,
				      NULL);
}

void
finish_labels (Compiler *compiler)
{
  hash_free (compiler->labels);
}

Label *
label_intern (Compiler *compiler, Object name, Block *block)
{
  Label *label = ALLOC (Label);
  label->name = name;
  Label *interned_label = hash_insert (compiler->labels, label);
  if (interned_label == NULL)
    xalloc_die ();
  if (label != interned_label)
    {
      if (block != NULL)
	{
	  if (interned_label->block != NULL)
	    compiler_error ("%s: %s", "duplicate label", object_get_str (name));
	  interned_label->block = block;
	}
      return interned_label;
    }

  label->block = block;
  return label;
}

static bool
check_label_processor (Label *label, Compiler *compiler)
{
  if (label->block == NULL)
    compiler_error ("%s: %s", "undefined label", object_get_str (label->name));
  return true;
}

void
check_labels (Compiler *compiler)
{
  hash_do_for_each (compiler->labels, (Hash_processor) check_label_processor,
		    compiler);
}

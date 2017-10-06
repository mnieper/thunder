/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdbool.h>
#include <stddef.h>

#include "common.h"
#include "vector.h"

#define LOC  VARIABLE_PARALLEL_COPY_LOC
#define PRED VARIABLE_PARALLEL_COPY_PRED
#define SEEN VARIABLE_PARALLEL_COPY_SEEN

DEFINE_VECTOR (Worklist, Variable *, worklist)

void
sequentialize_parallel_copy (Compiler *compiler, Instruction *move)
{
  // Idea: Put the emitted copies in two variable lists; replace
  // the arguments to the parallel move; interpret as sequential move.

  VariableList new_dests = variable_list_create (false);
  VariableList new_sources = variable_list_create (false);

  Worklist ready, todo;
  worklist_init (&ready);
  worklist_init (&todo);

  dest_source_foreach (dest, source, move)
    {
      LOC (dest) = NULL;
      PRED (source) = NULL;
      SEEN (dest) = false;
    }

  dest_source_foreach (dest, source, move)
    {
      if (SEEN (dest))
	continue;
      SEEN (dest) = true;
      if (dest == source)
	continue;
      LOC (source) = source;
      PRED (dest) = source;
      worklist_push (&todo, &dest);
    }

  dest_foreach (dest, move)
    SEEN (dest) = false;

  dest_source_foreach (dest, source, move)
    {
      if (SEEN (dest))
	continue;
      SEEN (dest) = true;
      if (dest == source)
	continue;
      if (LOC (dest) == NULL)
	worklist_push (&ready, &dest);
    }

  while (!worklist_empty (&todo))
    {
      Variable **b;
      while ((b = worklist_pop (&ready)) != NULL)
	{
	  Variable *a = PRED (*b);
	  Variable *c = LOC (a);
	  variable_list_add (new_dests, *b);
	  variable_list_add (new_sources, c);
	  LOC (a) = *b;
	  if (a == c && PRED (a) != NULL)
	    worklist_push (&ready, &a);
	}
      b = worklist_pop (&todo);
      if (*b == LOC (*b))
	{
	  Variable *n = program_create_var (compiler);
	  variable_list_add (new_dests, n);
	  variable_list_add (new_sources, *b);
	  LOC (*b) = n;
	  worklist_push (&ready, b);
	}
    }
  worklist_destroy (&ready);
  worklist_destroy (&todo);

  instruction_replace_dests (move, new_dests);
  instruction_replace_sources (move, new_sources);
}
#undef LOC
#undef PRED
#undef SEEN

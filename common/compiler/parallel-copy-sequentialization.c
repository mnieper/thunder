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

static void
add_move (Compiler *compiler, Block *block, InstructionListPosition* pos,
	  Variable *dest, Variable *source);

#define ADD_MOVE(dest, source)				\
  (add_move (compiler, block, pos, (dest), (source)))
void
sequentialize_parallel_copy (Compiler *compiler,
			     Block *block,
			     InstructionListPosition *pos,
			     Instruction *move)
{
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
	  ADD_MOVE (*b, c);
	  LOC (a) = *b;
	  if (a == c && PRED (a) != NULL)
	    worklist_push (&ready, &a);
	}
      b = worklist_pop (&todo);
      if (*b == LOC (*b))
	{
	  Variable *n = program_create_var (compiler);
	  ADD_MOVE (n, *b);
	  LOC (*b) = n;
	  worklist_push (&ready, b);
	}
    }
  worklist_destroy (&ready);
  worklist_destroy (&todo);

  block_remove_instruction (block, pos);
}
#undef ADD_MOVE

void
add_move (Compiler *compiler, Block *block, InstructionListPosition* pos,
	  Variable *dest, Variable *source)
{
  Instruction *ins
    = block_add_instruction_before (compiler, block, pos, get_opcode_movr ());
  instruction_add_dest (ins, dest);
  instruction_add_source (ins, source);
}

#undef LOC
#undef PRED
#undef SEEN

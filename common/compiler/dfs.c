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
# include <config.h>
#endif

#include "assure.h"
#include "common.h"
#include "vector.h"
#include "xalloc.h"

DEFINE_VECTOR (Worklist, Block *, worklist)

static void clear_visited_blocks (Program *program);
static bool block_visited (Block *block);
static void init_back_edges (Program *program);

#define PROGRAM (&(compiler)->program)

void
program_init_dfs (Compiler *compiler)
{
  clear_visited_blocks (PROGRAM);

  size_t blocks = block_count (PROGRAM);
  PROGRAM_PREORDER (PROGRAM) = COMPILER_NALLOC (compiler, blocks, Block *);
  PROGRAM_POSTORDER (PROGRAM) = COMPILER_NALLOC (compiler, blocks, Block *);

  Worklist worklist;
  worklist_init (&worklist);

  program_block_foreach (block, PROGRAM)
    if (block_root (block))
      worklist_push (&worklist, &block);

  size_t preindex = 0;
  size_t postindex = blocks;
  while (!worklist_empty (&worklist))
    {
      Block **block = worklist_top (&worklist);
      if (block_visited (*block))
	{
	  PROGRAM_POSTORDER (PROGRAM)[--postindex] = *block;
	  BLOCK_POSTINDEX (*block) = postindex;
	  worklist_pop (&worklist);
	}
      else
	{
	  PROGRAM_PREORDER (PROGRAM)[preindex] = *block;
	  BLOCK_PREINDEX (*block) = preindex++;
	  successor_foreach (succ, *block)
	    if (!block_visited (succ))
	      worklist_push (&worklist, &succ);
	}
    }
  assure (preindex == blocks);
  assure (postindex == 0);

  worklist_destroy (&worklist);

  init_back_edges (PROGRAM);
}

bool
program_back_edge (Block *source, Block *target)
{
  return source->postindex > target->postindex;
}

static void
clear_visited_blocks (Program *program)
{
  program_block_foreach (block, program)
    BLOCK_PREINDEX (block) = -1;
}

static bool
block_visited (Block *block)
{
  return BLOCK_PREINDEX (block) >= 0;
}

static void
init_back_edges (Program *program)
{
  preorder_foreach (block, program)
    {
      predecessor_foreach (pred, block)
	if (program_back_edge (pred, block))
	  {
	    edge_vector_push (program_back_edges (program),
			      &(Edge) { pred, block });
	    BLOCK_BACK_EDGE_TARGET (block) = true;
	  }
    }
}

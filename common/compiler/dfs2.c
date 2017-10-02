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
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdbool.h>

#include "assure.h"
#include "common.h"
#include "vector2.h"
#include "xalloc.h"

DEFINE_VECTOR(Worklist, Block *, worklist)

static void clear_visited_blocks (Program *);
static bool block_visited (Block *);
static void add_root_blocks (Worklist *, Program *);
static void init_back_edges (Program *);

void
program_init_dfs_tree (Program *program)
{
  clear_visited_blocks (program);

  size_t blocks = block_count (program);
  program->preorder = XNMALLOC (blocks, Block *);
  program->reverse_postorder = XNMALLOC (blocks, Block *);

  Worklist worklist;
  worklist_init (&worklist);
  add_root_blocks (&worklist, program);

  size_t preorder_number = 0;
  size_t reverse_postorder_number = blocks;
  while (!worklist_empty (&worklist))
    {
      Block **block = worklist_top (&worklist);
      if (block_visited (*block))
	{
	  program->reverse_postorder[--reverse_postorder_number] = *block;
	  (*block)->reverse_postorder_number = reverse_postorder_number;
	  worklist_pop (&worklist);
	}
      else
	{
	  program->preorder[preorder_number] = *block;
	  /* Setting the preorder number to a (non negative) value
	     marks the block as visited. */
	  (*block)->preorder_number = preorder_number++;

	  successor_foreach (succ, *block)
	    if (!block_visited (succ))
	      worklist_push (&worklist, &succ);
	}
    }
  assure (preorder_number == blocks);
  assure (reverse_postorder_number == 0);

  worklist_destroy (&worklist);

  init_back_edges (program);
}

bool
program_back_edge (Block *source, Block *target)
{
  return source->reverse_postorder_number > target->reverse_postorder_number;
}

static void
clear_visited_blocks (Program *program)
{
  block_foreach (block, program)
    block->preorder_number = -1;
}

static bool
block_visited (Block *block)
{
  return block->preorder_number >= 0;
}

static void
add_root_blocks (Worklist *worklist, Program *program)
{
  block_foreach (block, program)
    if (block_root (block))
      worklist_push (worklist, &block);
}

static void
init_back_edges (Program *program)
{
  preorder_foreach (block, program)
    {
      predecessor_foreach (pred, block)
	if (program_back_edge (pred, block))
	  {
	    edge_vector_push (&program->back_edges, &(Edge) { pred, block });
	    block->back_edge_target = true;
	  }
    }
}

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
#include <stddef.h>

#include "assure.h"
#include "common.h"
#include "minmax.h"
#include "vector2.h"

DEFINE_VECTOR (Worklist, Block *, worklist)

#define worklist_foreach(b, w)				\
  vector_foreach(b, Worklist, Block *, worklist, w)

static void dataflow (Program *);
static bool local_flow (Program *, Block *block);
static void init_dom_order (Program *);
static void init_max_dom (Program *);

void
program_init_dominance_tree (Program *program)
{
  size_t blocks = block_count (program);
  block_foreach (block, program)
    /* No block can be its own immediate dominator, so we use this
       value as a marker. */
    block->idom = block;

  dataflow (program);

  init_dom_order (program);

  init_max_dom (program);
}

bool
block_dominates (Block *block1, Block *block2)
{
  return (block2->dom_order_number >= block1->dom_order_number &&
	  block2->dom_order_number <= block1->max_dom_number);
}

static void
dataflow (Program *program)
{
  Worklist old_worklist, new_worklist, tmp_worklist;
  worklist_init (&old_worklist);
  worklist_init (&new_worklist);
  reverse_postorder_foreach (block, program)
    worklist_push (&new_worklist, &block);

  while (!worklist_empty (&new_worklist))
    {
      tmp_worklist = old_worklist;
      old_worklist = new_worklist;
      new_worklist = tmp_worklist;

      worklist_foreach (block, &old_worklist)
	if (local_flow (program, *block))
	  worklist_push (&new_worklist, block);

      worklist_clear (&old_worklist);
    }

  worklist_destroy (&old_worklist);
  worklist_destroy (&new_worklist);
}

static bool
local_flow (Program *program, Block *block)
{
  if (block_root (block))
    {
      block->idom = NULL;
      return false;
    }

  Block *new_idom = block;
  predecessor_foreach (pred, block)
    {
      if (pred->idom == pred)
	continue;
      if (new_idom == block)
	{
	  new_idom = pred;
	  continue;
	}
      Block *idom2 = pred->idom;
      size_t number1 = new_idom->reverse_postorder_number;
      size_t number2 = idom2->reverse_postorder_number;
      while (new_idom != idom2)
	{
	  while (idom2 != NULL
		 && number1 < (number2 = idom2->reverse_postorder_number))
	    idom2 = idom2->idom;
	  while (new_idom != NULL
		 && number2 < (number1 = new_idom->reverse_postorder_number))
	    new_idom = new_idom->idom;
	}
    }
  if (block->idom == new_idom)
    return false;
  block->idom = new_idom;
  return true;
}

static void
init_dom_order (Program *program)
{
  Worklist worklist;
  worklist_init (&worklist);

  block_foreach (block, program)
    {
      if (block->idom != NULL)
	block_list_add (block->idom->dom_children, block);
      else
	worklist_push (&worklist, &block);
    }

  size_t blocks = block_count (program);
  program->dom_order = XNMALLOC (blocks, Block *);

  size_t dom_order_number = 0;
  Block **block;
  while ((block = worklist_pop (&worklist)) != NULL)
    {
      program->dom_order[dom_order_number] = *block;
      (*block)->dom_order_number = dom_order_number++;

      dom_child_foreach (child, *block)
	worklist_push (&worklist, &child);
    }
  assure (dom_order_number == blocks);

  worklist_destroy (&worklist);
}

static void
init_max_dom (Program *program)
{
  block_foreach (block, program)
    block->max_dom_number = block->dom_order_number;

  postorder_foreach (block, program)
    {
      Block *idom = block->idom;
      if (idom != NULL)
	idom->max_dom_number = MAX (idom->max_dom_number,
				    block->max_dom_number);
    }
}

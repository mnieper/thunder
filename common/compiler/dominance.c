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
#include "minmax.h"
#include "vector.h"
#include "xalloc.h"

static void dominance_dataflow (Program *program);
static bool local_flow (Program *program, Block *block);
static void init_domindexes (Compiler *compiler);
static void init_max_domindexes (Program *program);

DEFINE_VECTOR (Worklist, Block *, worklist)

#define worklist_foreach(b, w)				\
  vector_foreach(b, Worklist, Block *, worklist, w)

#define PROGRAM (&(compiler)->program)

void
program_init_dominance (Compiler *compiler)
{
  program_block_foreach (block, PROGRAM)
     /* No block can be its own immediate dominator, so we use this
       value as a marker. */
    BLOCK_IDOM (block) = block;
  dominance_dataflow (PROGRAM);

  init_domindexes (compiler);
  init_max_domindexes (PROGRAM);
}

static void dominance_dataflow (Program *program)
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
      BLOCK_IDOM (block) = NULL;
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
      Block *idom2 = BLOCK_IDOM (pred);
      size_t number1 = BLOCK_POSTINDEX (new_idom);
      size_t number2 = BLOCK_POSTINDEX (idom2);
      while (new_idom != idom2)
	{
	  while (idom2 != NULL
		 && number1 < (number2 = BLOCK_POSTINDEX (idom2)))
	    idom2 = BLOCK_IDOM (idom2);
	  while (new_idom != NULL
		 && number2 < (number1 = BLOCK_POSTINDEX (new_idom)))
	    new_idom = BLOCK_IDOM (new_idom);
	}
    }
  if (BLOCK_IDOM (block) == new_idom)
    return false;
  BLOCK_IDOM (block) = new_idom;
  return true;
}

void
init_domindexes (Compiler *compiler)
{
  Worklist worklist;
  worklist_init (&worklist);

  program_block_foreach (block, PROGRAM)
    {
      if (BLOCK_IDOM (block) != NULL)
	block_list_add (block_dom_children (BLOCK_IDOM (block)), block);
      else
	worklist_push (&worklist, &block);
    }

  size_t blocks = block_count (PROGRAM);
  PROGRAM_DOMORDER (PROGRAM) = COMPILER_NALLOC (compiler, blocks, Block *);

  size_t domindex = 0;
  Block **block;
  while ((block = worklist_pop (&worklist)) != NULL)
    {
      PROGRAM_DOMORDER (PROGRAM)[domindex] = *block;
      BLOCK_DOMINDEX (*block) = domindex++;

      dom_child_foreach (child, *block)
	worklist_push (&worklist, &child);
    }
  assure (domindex == blocks);

  worklist_destroy (&worklist);
}

void
init_max_domindexes (Program *program)
{
  program_block_foreach (block, program)
    BLOCK_MAX_DOMINDEX (block) = BLOCK_DOMINDEX (block);

  postorder_foreach (block, program)
    {
      Block *idom = BLOCK_IDOM (block);
      if (idom != NULL)
	BLOCK_MAX_DOMINDEX (block) = MAX (BLOCK_MAX_DOMINDEX (block),
					  BLOCK_MAX_DOMINDEX (idom));
    }
}

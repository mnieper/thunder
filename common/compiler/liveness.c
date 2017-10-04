/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stddef.h>

#include "bitset.h"
#include "common.h"

static void add_use (Block *block, Variable *var, size_t time);
static void add_def (Block *block, Variable *var, size_t varindex, size_t time);
static void init_liveness_r (Program *program);
static void init_liveness_t (Program *program);

void
program_init_liveness (Program *program)
{
  init_liveness_r (program);
  init_liveness_t (program);
}

void
program_init_def_use_chains (Program *program)
{
  size_t varindex = 0;
  size_t time = 0;
  domorder_foreach (block, program)
    {
      phi_foreach (phi, block)
	{
	  source_foreach (var, phi)
	    add_use (block, var, time);
	  dest_foreach (var, phi)
	    add_def (block, var, varindex++, time + 1);
	}
      time++;
      block_instruction_foreach (ins, block)
	{
	  source_foreach (var, ins)
	    add_use (block, var, time);
	  time++;
	  dest_foreach (var, ins)
	    add_def (block, var, varindex++, time);
	}
    }
}

static void
add_use (Block *block, Variable *var, size_t time)
{
  UseChain *uses = variable_use_chain (var);

  Use *use = use_chain_top (uses);
  if (use == NULL || use->block != block)
    {
      use_chain_push (uses, &(Use) { .block = block });
      use = use_chain_top (uses);
    }
  use->last_use_time = time;
}

static void
add_def (Block *block, Variable *var, size_t varindex, size_t time)
{
  VARIABLE_DEF_BLOCK (var) = block;
  VARIABLE_INDEX (var) = varindex;
  VARIABLE_DEF_TIME (var) = time;
}

static void
init_liveness_r (Program *program)
{
  size_t blocks = block_count (program);
  program_block_foreach (block, program)
    BLOCK_LIVENESS_R (block) = bitset_create (blocks);

  postorder_foreach (block, program)
    {
      bitset_set_at (BLOCK_LIVENESS_R (block), BLOCK_DOMINDEX (block), true);
      successor_foreach (succ, block)
	{
	  if (succ == block || program_back_edge (block, succ))
	    continue;
	  bitset_union (BLOCK_LIVENESS_R (block), BLOCK_LIVENESS_R (succ));
	}
    }
}

static void
init_liveness_t (Program *program)
{
  size_t blocks = block_count (program);
  program_block_foreach (block, program)
    BLOCK_LIVENESS_T (block) = bitset_create (blocks);

  for (Edge *edge = edge_vector_begin (program_back_edges (program));
       edge != edge_vector_end (program_back_edges (program));
       edge++)
    {
      Block *target = edge->target;
      bitset_set_at (BLOCK_LIVENESS_T (target), BLOCK_DOMINDEX (target), true);
      for (Edge *e = edge_vector_begin (program_back_edges (program));
	   e != edge;
	   e++)
	if (bitset_at (BLOCK_LIVENESS_R (target), BLOCK_DOMINDEX (e->source))
	    && !bitset_at (BLOCK_LIVENESS_R (target),
			   BLOCK_DOMINDEX (e->target)))
	  bitset_union (BLOCK_LIVENESS_T (target),
			BLOCK_LIVENESS_T (e->target));
    }

  for (Edge *edge = edge_vector_begin (program_back_edges (program));
       edge != edge_vector_end (program_back_edges (program));
       edge++)
    bitset_union (BLOCK_LIVENESS_T (edge->source),
		  BLOCK_LIVENESS_T (edge->target));

  postorder_foreach (block, program)
    {
      successor_foreach (succ, block)
	if (!program_back_edge (block, succ))
	  bitset_union (BLOCK_LIVENESS_T (block), BLOCK_LIVENESS_T (succ));
      bitset_diff (BLOCK_LIVENESS_T (block), BLOCK_LIVENESS_R (block));
    }

  for (size_t i = 0; i < blocks; i++)
    bitset_set_at (BLOCK_LIVENESS_T (PROGRAM_DOMORDER (program)[i]), i, true);
}

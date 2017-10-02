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
#include <stdlib.h>

#include "bitset.h"
#include "common.h"
#include "vector2.h"

static void init_liveness_r (Program *);
static void init_liveness_t (Program *);
static bool use_outside_def (Variable *);
static bool block_live (Program *, Block *, Variable *, Block *);
static void init_def_use_chains (Program *);
static bool last_use (Block *, Variable *, size_t *);
static void add_use (Block *, Variable *, size_t);

void
program_init_liveness (Program *program)
{
  init_liveness_r (program);
  init_liveness_t (program);
  /* TODO(XXX): The following should be factored out because it
     depends also on the variables.  */
  init_def_use_chains (program);
}

bool
block_live_in (Program *program, Block *block, Variable *var)
{
  return block_live (program, block, var, NULL);
}

bool
block_live_out (Program *program, Block *block, Variable *var)
{
  if (block == var->def_block)
    return use_outside_def (var);

  return block_live (program, block, var,
		     block->back_edge_target ? NULL : block);
}

/* Return true if VAR is live in BLOCK at TIME.  It is assumed that
   the definition of VAR dominates the BLOCK and TIME.  */
bool
variable_live_at (Program *program, Block *block, size_t time, Variable *var)
{
  if (block_live_out (program, block, var))
    return true;

  if (!block_live_in (program, block, var) && var->def_block != block)
    return false;

  size_t last_time;
  if (!last_use (block, var, &last_time))
    return false;

  return last_time >= time;
}

static void
init_liveness_r (Program *program)
{
  size_t blocks = block_count (program);
  block_foreach (block, program)
    block->liveness_r = bitset_create (blocks);

  postorder_foreach (block, program)
    {
      bitset_set_at (block->liveness_r, block->dom_order_number, true);
      successor_foreach (succ, block)
	{
	  if (succ == block || program_back_edge (block, succ))
	    continue;
	  bitset_union (block->liveness_r, succ->liveness_r);
	}
    }
}

static void
init_liveness_t (Program *program)
{
  size_t blocks = block_count (program);
  block_foreach (block, program)
    block->liveness_t = bitset_create (blocks);

  for (Edge *edge = edge_vector_begin (&program->back_edges);
       edge != edge_vector_end (&program->back_edges);
       edge++)
    {
      Block *target = edge->target;
      bitset_set_at (target->liveness_t, target->dom_order_number, true);
      for (Edge *e = edge_vector_begin (&program->back_edges);
	   e != edge;
	   e++)
	if (bitset_at (target->liveness_r, e->source->dom_order_number)
	    && !bitset_at (target->liveness_r, e->target->dom_order_number))
	  bitset_union (target->liveness_t, e->target->liveness_t);
    }

  for (Edge *edge = edge_vector_begin (&program->back_edges);
       edge != edge_vector_end (&program->back_edges);
       edge++)
    bitset_union (edge->source->liveness_t, edge->target->liveness_t);

  postorder_foreach (block, program)
    {
      successor_foreach (succ, block)
	if (!program_back_edge (block, succ))
	  bitset_union (block->liveness_t, succ->liveness_t);
      bitset_diff (block->liveness_t, block->liveness_r);
    }

  for (size_t i = 0; i < blocks; i++)
    bitset_set_at (program->dom_order[i]->liveness_t, i, true);
}

static bool
block_live (Program *program, Block *block, Variable *var, Block *ignore)
{
  Block *def_block = var->def_block;
  size_t max_dom_number = def_block->max_dom_number;

  if (block->dom_order_number <= def_block->dom_order_number
      || block->dom_order_number > max_dom_number)
    return false;

  Block *b;
  for (size_t i = def_block->dom_order_number + 1;
       i <= max_dom_number
	 && (i = bitset_next (block->liveness_t, i, true)) <= max_dom_number;
       i = b->max_dom_number + 1)
    {
      b = program->dom_order[i];
      use_foreach(use, var)
	if ((b != ignore || use->block != ignore)
	    && bitset_at (b->liveness_r, use->block->dom_order_number))
	  return true;
    }
  return false;
}

/* Return true if VAR is used in another block as the one where it is
   defined. */
static bool
use_outside_def (Variable *var)
{
  if (use_chain_empty (&var->use_chain))
    return false;
  if (use_chain_begin (&var->use_chain)->block == var->def_block)
    return use_chain_size (&var->use_chain) > 0;
  return true;
}

static void
init_def_use_chains (Program *program)
{
  size_t var_number = 0;
  size_t ins_number = 0;
  size_t i = 0;
  dom_order_foreach (block, program)
    {
      phi_foreach (phi, block)
	{
	  source_foreach (var, phi)
	    add_use (block, var, ins_number);
	  dest_foreach (var, phi)
	    {
	      var->def_block = block;
	      var->dom_number = var_number++;
	      var->def_number = ins_number + 1;
	    }
	}
      ins_number++;
      instruction_foreach (ins, block)
	{
	  source_foreach (var, ins)
	    add_use (block, var, ins_number);
	  ins_number++;
	  dest_foreach (var, ins)
	    {
	      var->def_block = block;
	      var->dom_number = var_number++;
	      var->def_number = ins_number;
	    }
	}
    }
}

static void
add_use (Block *block, Variable *var, size_t number)
{
  Use *use = use_chain_top (&var->use_chain);
  if (use == NULL || use->block != block)
    {
      use_chain_push (&var->use_chain, &(Use) { .block = block });
      use = use_chain_top (&var->use_chain);
    }
  use->last_use = number;
}

static int compare_use (const Block *, const Use *);

static bool
last_use (Block *block, Variable *var, size_t *time)
{
  Use *use = bsearch (block,
		      use_chain_begin (&var->use_chain),
		      use_chain_size (&var->use_chain),
		      sizeof (Use),
		      (int (*) (const void *, const void *)) compare_use);
  if (use == NULL)
    return false;
  *time = use->last_use;
  return true;
}

static int compare_use (const Block *block, const Use *use)
{
  if (block->dom_order_number == use->block->dom_order_number)
    return 0;
  if (block->dom_order_number < use->block->dom_order_number)
    return -1;
  return 1;
}

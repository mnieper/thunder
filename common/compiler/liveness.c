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
#include <stdlib.h>

#include "bitset.h"
#include "common.h"

static void add_use (Block *block, Variable *var, size_t time);
static void add_def (Block *block, Variable *var, size_t varindex, size_t time);
static void init_liveness_r (Program *program);
static void init_liveness_t (Program *program);
static bool live_in (Program *program, Block *block, Variable *var);
static bool live_out (Program *program, Block *block, Variable *var);
static bool live (Program *program, Block *block, Variable *var, Block *ignore);
static bool use_outside_def (Variable *var);
static bool last_use_time (Block *block, Variable *var, size_t *time);

/* TODO: Coalesce with same macro in ssa-destruction.c */
#define phi_source_foreach(var, pred, pos, block, phi)			\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (VariableListIterator i##__LINE__				\
	   = variable_list_iterator ((phi)->sources);			\
	 b##__LINE__; )							\
      for (BlockListIterator j##__LINE__				\
	     = block_list_iterator ((block)->predecessors);		\
	   b##__LINE__;)						\
	for (VariableListPosition *(pos); b##__LINE__;)			\
	  for (Block *(pred); b##__LINE__;				\
	       b##__LINE__ = false,					\
		 variable_list_iterator_free (&i##__LINE__),		\
		 block_list_iterator_free (&j##__LINE__))		\
	    for (Variable *(var);					\
		 variable_list_iterator_next (&i##__LINE__, &var, &pos)	\
		   && (block_list_iterator_next (&j##__LINE__, &pred, NULL) \
		       || true); )


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
	  phi_source_foreach (var, pred, pos, block, phi)
	    add_use (pred, var, time);
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

bool
variable_live_at (Program *program, Block *block, size_t time,
		  Variable *v)
{
  if (live_out (program, block, v))
    return true;

  if (VARIABLE_DEF_BLOCK (v) != block && !live_in (program, block, v))
    return false;

  size_t time_of_last_use;
  if (!last_use_time (block, v, &time_of_last_use))
    return false;

  return time_of_last_use >= time;
}

bool
live_in (Program *program, Block *block, Variable *var)
{
  bool res = live (program, block, var, NULL);
#ifdef DEBUG
  fprintf (stderr, "VM: live_in (preindex=%tu, domindex=%tu): %u\n",
	   BLOCK_PREINDEX (block), VARIABLE_INDEX (var), res);
#endif
  return res;
}

bool
live_out (Program *program, Block *block, Variable *var)
{
  bool res;

  if (block == VARIABLE_DEF_BLOCK (var))
    return use_outside_def (var);
  else
    res = live (program, block, var,
		BLOCK_BACK_EDGE_TARGET (block) ? NULL : block);

#ifdef DEBUG
  fprintf (stderr, "VM: live_out (preindex=%tu, domindex=%tu): %u\n",
	   BLOCK_PREINDEX (block), VARIABLE_INDEX (var), res);
#endif
  return res;
}

bool
live (Program *program, Block *block, Variable *var, Block *ignore)
{
  Block *def_block = VARIABLE_DEF_BLOCK (var);
  size_t max_domindex = BLOCK_MAX_DOMINDEX (def_block);

  if (BLOCK_DOMINDEX (block) <= BLOCK_DOMINDEX (def_block)
      || BLOCK_DOMINDEX (block) > max_domindex)
    return false;

  Block *b;
  for (size_t i = BLOCK_DOMINDEX (def_block) + 1;
       i <= max_domindex
	 && ((i = bitset_next (BLOCK_LIVENESS_T (block), i, true))
	     <= max_domindex);
       i = BLOCK_MAX_DOMINDEX (b) + 1)
    {
      b = PROGRAM_DOMORDER (program)[i];
      use_foreach(use, var)
	if ((b != ignore || use->block != ignore)
	    && bitset_at (BLOCK_LIVENESS_R (b), BLOCK_DOMINDEX (use->block)))
	  return true;
    }
  return false;
}

bool
use_outside_def (Variable *var)
{
  UseChain *uses = variable_use_chain (var);

  if (use_chain_empty (uses))
    return false;
  if (use_chain_begin (uses)->block == VARIABLE_DEF_BLOCK (var))
    return use_chain_size (uses) > 1;
  return true;
}

static int compare_use (const Block *block, const Use *use);

bool
last_use_time (Block *block, Variable *var, size_t *time)
{
  Use *use = bsearch (block,
		      use_chain_begin (&var->use_chain),
		      use_chain_size (&var->use_chain),
		      sizeof (Use),
		      (int (*) (const void *, const void *)) compare_use);
  if (use == NULL)
    return false;
  *time = use->last_use_time;
  return true;
}

int compare_use (const Block *block, const Use *use)
{
  if (BLOCK_DOMINDEX (block) == BLOCK_DOMINDEX (use->block))
    return 0;
  if (BLOCK_DOMINDEX (block) < BLOCK_DOMINDEX (use->block))
    return -1;
  return 1;
}

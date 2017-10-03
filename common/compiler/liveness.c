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

static void init_liveness_r (Program *program);
static void init_liveness_t (Program *program);

void
program_init_liveness (Program *program)
{
  init_liveness_r (program);
  init_liveness_t (program);
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

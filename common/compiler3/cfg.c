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
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "compiler/common.h"
#include "compiler/cfg.h"
#include "graph.h"

#define LOOP_LENGTH 32000

void
init_cfg (Cfg *cfg)
{
  graph_init (CFG, cfg);
}

void
finish_cfg (Cfg *cfg)
{
  graph_destroy (CFG, cfg);
}

void
cfg_insert_block (Cfg *cfg, Block *block)
{
  block->length = 0;
  graph_add_node (CFG, cfg, block);
}

Block *
cfg_next_block (Block *block)
{
  return graph_next_node (CFG, block);
}

void
cfg_add_link (Link *link, Block *source, Block *target)
{
  graph_add_edge (CFG, link, source, target);
}

void
cfg_split_critical_edges (Cfg *cfg, cfg_edge_splitter splitter, void *data)
{
  graph_split_critical_edges (CFG, cfg, splitter, data);
}

void
cfg_set_length (Link *link, size_t length)
{
  edge_node (CFG, link)->length = length;
}

void
cfg_set_lengths (Cfg *cfg)
{
  block_foreach (block, cfg)
    {
      link_foreach (link, block)
	{
	  Block *source = edge_source (CFG, link);
	  Block *target = edge_target (CFG, link);
	  if (node_loop_header (CFG, source) != node_loop_header (CFG, target))
	    cfg_set_length (link, LOOP_LENGTH);
	}
    }
}

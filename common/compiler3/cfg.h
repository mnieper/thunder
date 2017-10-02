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

#ifndef COMPILER_CFG_H_INCLUDED
#define COMPILER_CFG_H_INCLUDED

#include "common.h"

#define CFG GRAPH(Block, blocks, Link, links)

typedef EDGE_SPLITTER_DECL(CFG, cfg_edge_splitter);

#define block_foreach(block, cfg)		\
  node_foreach (block, CFG, cfg)

#define link_foreach(edge, node)		\
  edge_outgoing_foreach(edge, CFG, node)

void init_cfg (Cfg *cfg);
void finish_cfg (Cfg *cfg);

void cfg_insert_block (Cfg *cfg, Block *block);
Block *cfg_next_block (Block *block);

void cfg_add_link (Link *link, Block *source, Block *target);

void cfg_split_critical_edges (Cfg *cfg, cfg_edge_splitter splitter,
			       void *data);

void cfg_set_length (Link *link, size_t length);
void cfg_set_lengths (Cfg *cfg);

#endif /* COMPILER_CFG_H_INCLUDED */

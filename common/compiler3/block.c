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
#include <stdbool.h>
#include <stddef.h>

#include "deque.h"

#include "compiler/block.h"
#include "compiler/cfg.h"
#include "compiler/common.h"

Link *
new_link (Compiler *compiler, Value *source)
{
  Link *link = ALLOC (Link);
  link->source = source;
  return link;
}

Block *
new_block (Compiler *compiler, Object name)
{
  Block *block = ALLOC (Block);
  block->name = name;
  deque_init (INSTRUCTIONS, &block->instructions);
  cfg_insert_block (&compiler->cfg, block);
  return block;
}

void
block_append_instruction (Block *block, Instruction *ins)
{
  deque_insert_last (INSTRUCTIONS, &block->instructions, ins);
}

Instruction *
block_end (Block *block)
{
  return deque_last (INSTRUCTIONS, &block->instructions);
}

void
block_add_link (Compiler *compiler, Block *block, Value *source)
{
  Link *link = new_link (compiler, source);
  cfg_add_link (link, block, source->block);
}

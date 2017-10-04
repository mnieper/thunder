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
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifndef COMPILER_BLOCK_H_INCLUDED
#define COMPILER_BLOCK_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "bitset.h"
#include "instruction.h"
#include "list.h"
#include "opcode.h"

#define block_foreach(b, l)					\
  list_foreach (b, BlockList, Block, block_list, l)

#define successor_foreach(succ, block)					\
  list_foreach (succ, BlockList, Block, block_list, (block)->successors)

#define predecessor_foreach(pred, block)				\
  list_foreach (pred, BlockList, Block, block_list, (block)->predecessors)

#define BLOCK_PREINDEX(block)			\
  ((block)->preindex)
#define BLOCK_POSTINDEX(block)			\
  ((block)->postindex)
#define BLOCK_DOMINDEX(block)			\
  ((block)->domindex)
#define BLOCK_IDOM(block)			\
  ((block)->idom)
#define BLOCK_LIVENESS_R(block)			\
  ((block)->liveness_r)
#define BLOCK_LIVENESS_T(block)			\
  ((block)->liveness_t)
#define BLOCK_BACK_EDGE_TARGET(block)		\
  ((block)->back_edge_target)
#define BLOCK_MOVE_IN(block)			\
  ((block)->move_in)
#define BLOCK_MOVE_OUT(block)			\
  ((block)->move_out)

typedef struct block Block;

struct compiler;
struct instruction;

Block *block_create (struct compiler *compiler);
void block_free (Block *block);

static inline struct block_list block_dom_children (Block *block);
static inline InstructionListPosition *block_terminating (Block *block);

bool block_root (Block *block);

void block_add_successor (Block *source, Block *target);
void block_add_predecessor (Block *target, Block *source);
struct instruction *block_add_instruction (struct compiler *compiler,
					   Block *block,
					   const Opcode *opcode);
struct instruction *block_add_instruction_first (struct compiler *compiler,
						 Block *block,
						 const Opcode *opcode);
struct instruction *block_add_instruction_before (struct compiler *compiler,
						  Block *block,
						  InstructionListPosition *pos,
						  const Opcode *opcode);

DEFINE_LIST(BlockList, Block, block_list, block_free);

struct block
{
  InstructionList instructions;
  BlockList successors;
  BlockList predecessors;
  BlockList dom_children;
  InstructionListPosition *last_instruction;
  Instruction *move_in;
  Instruction *move_out;
  ptrdiff_t preindex;
  ptrdiff_t postindex;
  ptrdiff_t domindex;
  Block *idom;
  Bitset* liveness_r;
  /* TODO: Implement and use a SparseBitset for liveness_t.  */
  Bitset* liveness_t;
  bool back_edge_target : 1;
};

static inline BlockList
block_dom_children (Block *block)
{
  return block->dom_children;
}

static inline InstructionListPosition *
block_terminating (Block *block)
{
  return block->last_instruction;
}

#endif /* COMPILER_BLOCK_H_INCLUDED */

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

#ifdef DEBUG
# include <stdio.h>
#endif

#define block_foreach(b, l)					\
  list_foreach (b, BlockList, Block, block_list, l)

#define successor_foreach(succ, block)					\
  block_foreach (succ, (block)->successors)
#define predecessor_foreach(pred, block)					\
  block_foreach (pred, (block)->predecessors)

#define phi_foreach(phi, block)			\
  instruction_foreach (phi, (block)->phis)
#define block_instruction_foreach(ins, block)	\
  instruction_foreach (ins, (block)->instructions)

#define BLOCK_PREINDEX(block)			\
  ((block)->preindex)
#define BLOCK_POSTINDEX(block)			\
  ((block)->postindex)
#define BLOCK_DOMINDEX(block)			\
  ((block)->domindex)
#define BLOCK_MAX_DOMINDEX(block)		\
  ((block)->max_domindex)
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
#define BLOCK_DEFS(block)      ((block)->defs)
#define BLOCK_USES(block)      ((block)->uses)
#define BLOCK_LIVE_IN(block)   ((block)->live_in)
#define BLOCK_LIVE_OUT(block)  ((block)->live_out)
#define BLOCK_TIME_FROM(block) ((block)->time_from)
#define BLOCK_TIME_TO(block)   ((block)->time_to)

typedef struct block Block;

struct compiler;
struct instruction;

Block *block_create (struct compiler *compiler);
void block_free (Block *block);

static inline struct block_list block_dom_children (Block *block);
static inline InstructionListPosition *block_terminating (Block *block);
static inline InstructionListPosition *block_first_instruction (Block *block);
static inline InstructionListPosition *
block_next_instruction (Block *block, InstructionListPosition *pos);
static inline InstructionListPosition *
block_previous_instruction (Block *block, InstructionListPosition *pos);
static inline Instruction *block_instruction_at (Block *block,
						 InstructionListPosition *pos);

bool block_root (Block *block);
bool block_dominates (Block *a, Block *b);

void block_add_successor (Block *source, Block *target);
void block_add_predecessor (Block *target, Block *source);
struct instruction *block_add_phi_func (struct compiler *compiler,
					Block *block,
					const Opcode *opcode);
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
static void block_remove_instruction (Block *block,
				      InstructionListPosition *pos);

#ifdef DEBUG
void block_out_str (FILE *out, Block *block);
#endif

DEFINE_LIST(BlockList, Block, block_list, block_free);

struct block
{
  InstructionList instructions;
  InstructionList phis;
  BlockList successors;
  BlockList predecessors;
  BlockList dom_children;
  InstructionListPosition *last_instruction;
  Instruction *move_in;
  Instruction *move_out;
  ptrdiff_t preindex;
  ptrdiff_t postindex;
  ptrdiff_t domindex;
  ptrdiff_t max_domindex;
  Block *idom;
  Bitset *liveness_r;
  /* TODO: Implement and use a SparseBitset for liveness_t.  */
  Bitset *liveness_t;
  Bitset *defs;
  Bitset *uses;
  Bitset *live_in;
  Bitset *live_out;
  size_t time_from;
  size_t time_to;
  bool back_edge_target : 1;
};

inline BlockList
block_dom_children (Block *block)
{
  return block->dom_children;
}

inline InstructionListPosition *block_first_instruction (Block *block)
{
  return instruction_list_begin (block->instructions);
}

inline InstructionListPosition *
block_terminating (Block *block)
{
  return block->last_instruction;
}

inline InstructionListPosition *
block_next_instruction (Block *block, InstructionListPosition *pos)
{
  return instruction_list_next (block->instructions, pos);
}

inline InstructionListPosition *
block_previous_instruction (Block *block, InstructionListPosition *pos)
{
  return instruction_list_previous (block->instructions, pos);
}

inline Instruction *
block_instruction_at (Block *block, InstructionListPosition *pos)
{
  return instruction_list_at (block->instructions, pos);
}

inline void
block_remove_instruction (Block *block, InstructionListPosition *pos)
{
  instruction_list_remove (block->instructions, pos);
}

#endif /* COMPILER_BLOCK_H_INCLUDED */

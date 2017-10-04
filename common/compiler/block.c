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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "common.h"
#include "list.h"

Block *
block_create (Compiler *compiler)
{
  Block *block = COMPILER_ALLOC (compiler, Block);

  block->phis = instruction_list_create (true);
  block->instructions = instruction_list_create (true);
  block->successors = block_list_create (false);
  block->predecessors = block_list_create (false);
  block->dom_children = block_list_create (false);
  block->liveness_r = NULL;
  block->liveness_t = NULL;
  block->back_edge_target = false;

  return block;
}

void
block_free (Block *block)
{
  instruction_list_free (block->phis);
  instruction_list_free (block->instructions);
  block_list_free (block->successors);
  block_list_free (block->predecessors);
  block_list_free (block->dom_children);
  bitset_free (block->liveness_r);
  bitset_free (block->liveness_t);
}

bool
block_root (Block *block)
{
  return block_list_size (block->predecessors) == 0;
}

bool
block_dominates (Block *a, Block *b)
{
  return (BLOCK_DOMINDEX (a) <= BLOCK_DOMINDEX (b)
	  && BLOCK_DOMINDEX (b) <= BLOCK_MAX_DOMINDEX (a));
}

void
block_add_successor (Block *source, Block *target)
{
  block_list_add (source->successors, target);
}

void
block_add_predecessor (Block *target, Block *source)
{
  block_list_add (target->predecessors, source);
}

Instruction *
block_add_phi_func (Compiler *compiler, Block *block, const Opcode *opcode)
{
  Instruction *phi = instruction_create (compiler, opcode);
  instruction_list_add (block->phis, phi);
  return phi;
}

Instruction *
block_add_instruction (Compiler *compiler, Block *block, const Opcode *opcode)
{
  Instruction *ins = instruction_create (compiler, opcode);
  block->last_instruction = instruction_list_add (block->instructions, ins);
  return ins;
}

struct instruction *block_add_instruction_first (struct compiler *compiler,
						 Block *block,
						 const Opcode *opcode)
{
  Instruction *ins = instruction_create (compiler, opcode);
  instruction_list_add_first (block->instructions, ins);
  return ins;
}

struct instruction *block_add_instruction_before (struct compiler *compiler,
						  Block *block,
						  InstructionListPosition *pos,
						  const Opcode *opcode)
{
  Instruction *ins = instruction_create (compiler, opcode);
  instruction_list_add_before (block->instructions, pos, ins);
  return ins;
}

#ifdef DEBUG

void
block_out_str (FILE *out, Block *block){
  fprintf (out, "BLOCK (preindex=%tu, postindex=%tu, domindex=%tu)\n",
	   block->preindex, block->postindex, block->domindex);
  phi_foreach (phi, block)
    instruction_out_str (out, phi);
  block_instruction_foreach (ins, block)
    instruction_out_str (out, ins);
  successor_foreach (succ, block)
    fprintf (out, "Successor: %tu\n", succ->preindex);
}

#endif

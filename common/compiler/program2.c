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
#include <stdlib.h>

#include "common.h"
#include "xalloc.h"

static Block *create_block (void);
static Variable *create_variable (void);

void
program_init (Program *program)
{
  program->blocks = block_list_create (true);
  program->vars = variable_list_create (true);
  program->preorder = NULL;
  program->reverse_postorder = NULL;
  program->dom_order = NULL;
  edge_vector_init (&program->back_edges);
}

void
program_destroy (Program *program)
{
  block_list_free (program->blocks);
  variable_list_free (program->vars);
  edge_vector_destroy (&program->back_edges);
  free (program->preorder);
  free (program->reverse_postorder);
  free (program->dom_order);
}

size_t
block_count (Program *program)
{
  return block_list_size (program->blocks);
}

bool
block_root (Block *block)
{
  return block_list_size (block->predecessors) == 0;
}

void
block_free (Block *block)
{
  block_list_free (block->successors);
  block_list_free (block->predecessors);
  block_list_free (block->dom_children);
  instruction_list_free (block->phis);
  instruction_list_free (block->instructions);
  bitset_free (block->liveness_r);
  bitset_free (block->liveness_t);
  free (block);
}

InstructionListPosition *
block_add_instruction (Block *block, Instruction *ins)
{
  InstructionListPosition *pos
    = instruction_list_add (block->instructions, ins);
  block->last_instruction = pos;
  return pos;
}

InstructionListPosition *
block_add_instruction_first (Block *block, Instruction *ins)
{
  return instruction_list_add_first (block->instructions, ins);
}

InstructionListPosition *
block_add_instruction_before (Block *block, InstructionListPosition* pos,
			      Instruction *ins)
{
  return instruction_list_add_before (block->instructions, pos, ins);
}

Instruction *
block_add_phi (Block *block)
{
  Instruction *phi = instruction_phi ();
  instruction_list_add (block->phis, phi);
  return phi;
}

void
variable_free (Variable *var)
{
  use_chain_destroy (&var->use_chain);
  variable_vector_destroy (&var->congruence_class);
  free (var);
}

Block *
program_add_block (Program *program)
{
  Block *block = create_block ();
  block_list_add (program->blocks, block);
  return block;
}

Variable *
program_add_variable (Program *program)
{
  Variable *var = create_variable ();
  variable_list_add (program->vars, var);
  return var;
}

void
program_add_edge (Block *source, Block *target)
{
  block_list_add (source->successors, target);
  block_list_add (target->predecessors, source);
}

static Block*
create_block (void)
{
  Block *block = XMALLOC (Block);
  block->successors = block_list_create (false);
  block->predecessors = block_list_create (false);
  block->dom_children = block_list_create (false);
  block->phis = instruction_list_create (true);
  block->instructions = instruction_list_create (true);
  block->liveness_r = NULL;
  block->liveness_t = NULL;
  block->back_edge_target = false;
  return block;
}

static Variable *
create_variable ()
{
  Variable *var = XMALLOC (Variable);
  use_chain_init (&var->use_chain);
  variable_vector_init (&var->congruence_class);
  return var;
}

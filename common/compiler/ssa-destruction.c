/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as
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
#include <config.h>
#endif

#include "common.h"

static void add_move_instructions (Compiler *compiler);
static void isolate_phis (Compiler *compiler);
static void isolate_phi (Compiler *compiler, Block *block, Instruction *phi);
static void init_congruence_classes (Program *program);
static void merge_congruence_classes (Variable *v, Variable *w);
static void coalesce_phis (Program *program);
static void coalesce_phi (Program *program, Block *block, Instruction *phi);

#define PROGRAM (&compiler->program)

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
program_convert_out_of_ssa (Compiler *compiler)
{
  add_move_instructions (compiler);
  isolate_phis (compiler);
  program_init_def_use_chains (PROGRAM);
  init_congruence_classes (PROGRAM);
  coalesce_phis (PROGRAM);
}

void
add_move_instructions (Compiler *compiler)
{
  program_block_foreach (block, PROGRAM)
    {
      BLOCK_MOVE_IN (block)
	= block_add_instruction_first (compiler, block, get_opcode_movr ());
      BLOCK_MOVE_OUT (block)
	= block_add_instruction_before (compiler,
					block,
					block_terminating (block),
					get_opcode_movr ());
    }
}

void
isolate_phis (Compiler *compiler)
{
  program_block_foreach (block, PROGRAM)
    {
      phi_foreach (phi, block)
	isolate_phi (compiler, block, phi);
    }
}

void
isolate_phi (Compiler *compiler, Block *block, Instruction *phi)
{
  instruction_add_dest (BLOCK_MOVE_IN (block), instruction_dest (phi));
  Variable *var = program_create_var (compiler);
  instruction_add_source (BLOCK_MOVE_IN (block), var);
  instruction_set_dest (phi, var);

  phi_source_foreach (input, pred, pos, block, phi)
    {
      Variable *var = program_create_var (compiler);
      instruction_add_dest (BLOCK_MOVE_OUT (pred), var);
      instruction_add_source (BLOCK_MOVE_OUT (pred), input);
      instruction_replace_source (phi, pos, var);
    }
}

void
init_congruence_classes (Program *program)
{
  program_variable_foreach (var, program)
    {
      VARIABLE_CONGRUENCE (var) = var;
      variable_vector_push (variable_congruence_class (var), &var);
    }
}

void
coalesce_phis (Program *program)
{
  program_block_foreach (block, program)
    {
      phi_foreach (phi, block)
	coalesce_phi (program, block, phi);
    }
}

void
coalesce_phi (Program *program, Block *block, Instruction *phi)
{
  Variable *dest = instruction_dest (phi);
  source_foreach (source, phi)
    merge_congruence_classes (dest, source);
}

void
merge_congruence_classes (Variable *v, Variable *w)
{
  v = VARIABLE_CONGRUENCE (v);
  w = VARIABLE_CONGRUENCE (w);
  if (v == w)
    return;

  if (VARIABLE_INDEX (v) > VARIABLE_INDEX (w))
    {
      Variable *tmp = v;
      v = w;
      w = tmp;
    }

  VariableVector *c = variable_congruence_class (v);
  VariableVector *d = variable_congruence_class (w);
  VariableVector x = *c;
  variable_vector_init (c);

  Variable **p = variable_vector_begin (&x);
  Variable **q = variable_vector_begin (variable_congruence_class (w));

  while (p != variable_vector_end (&x) && q != variable_vector_end (d))
    if (VARIABLE_INDEX (*p) < VARIABLE_INDEX (*q))
      variable_vector_push (c, p++);
    else if (VARIABLE_INDEX (*p) > VARIABLE_INDEX (*q))
      {
	VARIABLE_CONGRUENCE (*q) = v;
	variable_vector_push (c, q++);
      }
    else
      {
	VARIABLE_CONGRUENCE (*q) = v;
	variable_vector_push (c, (p++, q++));
      }
  while (p != variable_vector_end (&x))
    variable_vector_push (c, p++);
  while (q != variable_vector_end (d))
    {
      VARIABLE_CONGRUENCE (*q) = v;
      variable_vector_push (c, q++);
    }

  variable_vector_destroy (&x);
  variable_vector_clear (d);
}
